// BLDC_driver_rp2040_main_buttons_annotated.c
// Vollständig annotierte Version deines BLDC 6‑Step Programms.
// Jede Zeile/Anweisung ist kommentiert, damit du den Code im Detail erklären kannst.

#include "hardware/adc.h"    // ADC-API (falls du später Strom/Spannung messen willst)
#include "hardware/clocks.h" // clock_get_hz() und andere clock-Funktionen
#include "hardware/gpio.h"   // GPIO-Funktionen (init, set_dir, put, get, pull_up ...)
#include "hardware/pwm.h"    // PWM-Funktionen (wrap, chan_level, slice, clkdiv ...)
#include "hardware/timer.h"  // Timer / Zeit-Funktionen (absolute Zeiten, sleep_us ...)
#include "pico/stdlib.h"     // Standard-Lib für RP2040 (stdio_init_all, sleep_ms, usw.)
#include <stdio.h>           // stdio (printf) für Debug-Ausgaben

// -------------------- Konfiguration (anpassen!) --------------------
// Hier definierst du die verwendeten Pins und grundlegende Parameter.
// Erkläre in der Arbeit: Warum diese Pins, welche Hardware hängt daran, Spannungspegel, etc.

// HS_PIN: Gate-Pins für die High-Side P‑MOSFETs (Phase A,B,C)
// In deinem Hardwareaufbau: MCU zieht Gate auf GND => P‑MOSFET schaltet ein.
// Wenn Pin hochohmig (Input) => externer Pullup zieht Gate auf 5V => P‑MOSFET aus.
const uint HS_PIN[3] = {2, 6, 10}; // HS für Phase A,B,C

// LS_PIN: PWM-Pins für die Low-Side N‑MOSFETs (Phase A,B,C)
// Hier läuft später die PWM (Funktion GPIO_FUNC_PWM)
const uint LS_PIN[3] = {3, 7, 11}; // LS für Phase A,B,C

// Buttons: Increase / Decrease (active LOW angenommen, deshalb Pull‑Up)
const uint BUTTON_INC_PIN = 12; // Taster schneller
const uint BUTTON_DEC_PIN = 13; // Taster langsamer

// Not-Aus / Fault Pins (hardwareseitig verbinden)
// ESTOP: active LOW → gedrückt = 0 = E‑Stop aktiv
const uint ESTOP_PIN = 14;
// FAULT: optionaler Fehler-Eingang (z. B. Überstromdetektor), hier active HIGH
const uint FAULT_PIN = 15;

// PWM Basis-Einstellungen:
// PWM_FREQ: gewünschte PWM-Frequenz (20 kHz ist üblich für Motorsteuerungen)
const uint PWM_FREQ = 20000; // 20 kHz
// PWM_WRAP: Auflösung der PWM (hier 16 bit)
const uint PWM_WRAP = 65535; // (2^16 - 1)

// DEAD_TIME_US: softwarebasierte Totzeit in Mikrosekunden, um Shoot‑Through zu vermeiden.
// Hinweise: Software-Deadtime ist ungenauer als hardwareseitige Deadtime. Sie muss an Gate‑Lade‑/Entladezeiten angepasst werden.
const uint32_t DEAD_TIME_US = 10; // microseconds (anpassen/vermessen!)

// Kommutations-Timing: steuert die Drehzahl im Open‑Loop.
// step_time_ms = Dauer einer Kommutationsstufe; kleiner -> schneller.
static uint32_t step_time_ms = 200;     // initial 200 ms pro Schritt
const uint32_t STEP_TIME_MIN_MS = 20;   // minimaler Wert (schnell)
const uint32_t STEP_TIME_MAX_MS = 2000; // maximaler Wert (sehr langsam)
const uint32_t STEP_TIME_STEP_MS = 20;  // Schrittweite pro Tastendruck

// Button Debounce / Auto-Repeat Zeiten
const uint32_t BUTTON_DEBOUNCE_MS = 50; // Entprellzeit
const uint32_t BUTTON_REPEAT_MS = 150;  // Wiederholintervall beim Halten

// Polling-Intervall während Wartezeit (macht Buttons reaktionsschnell)
const uint32_t POLL_MS = 10; // wie oft pro step_time Buttons geprüft werden

// -------------------- Interne Helpers --------------------
// Diese Hilfsfunktionen kapseln Hardwareaktionen (HS/LS ein/aus, PWM set).

// HS einschalten: MCU Pin als Output LOW setzen -> Gate auf 0V -> P‑MOSFET ON
static inline void hs_drive_on(unsigned phase)
{
    gpio_set_function(HS_PIN[phase], GPIO_FUNC_SIO); // setze Pin-Funktion auf SIO (Software controlled GPIO)
    gpio_set_dir(HS_PIN[phase], GPIO_OUT);           // Pin als Ausgang
    gpio_put(HS_PIN[phase], 0);                      // schreibe 0 (LOW) -> Gate auf 0V ziehen
}

// HS ausschalten: MCU Pin als Input setzen -> externes Pullup zieht Gate auf 5V -> P‑MOSFET OFF
static inline void hs_drive_off(unsigned phase)
{
    gpio_set_dir(HS_PIN[phase], GPIO_IN); // Pin hochohmig machen (Input) -> externes Pullup übernimmt
    // Achtung: interne Pulls sind deaktiviert (siehe init), wir verlassen uns auf externe 5V Pullups
}

// LS PWM deaktivieren: setze Duty auf 0 und Pin in sicheren Zustand (Input)
static inline void ls_pwm_disable(unsigned phase)
{
    uint slice = pwm_gpio_to_slice_num(LS_PIN[phase]);                // erhalte PWM-Slice (Hardware Einheit)
    pwm_set_chan_level(slice, pwm_gpio_to_channel(LS_PIN[phase]), 0); // Duty = 0 (kein PWM)
    gpio_set_function(LS_PIN[phase], GPIO_FUNC_SIO);                  // Pin zurück auf SIO (kein PWM)
    gpio_set_dir(LS_PIN[phase], GPIO_IN);                             // Pin hochohmig -> sicher
}

// LS PWM aktivieren: setze Pin-Funktion auf PWM und Duty-Level
static inline void ls_pwm_enable(unsigned phase, uint32_t level)
{
    gpio_set_function(LS_PIN[phase], GPIO_FUNC_PWM);                      // aktiviere PWM-Funktion für Pin
    uint slice = pwm_gpio_to_slice_num(LS_PIN[phase]);                    // Slice-Nummer abfragen
    pwm_set_chan_level(slice, pwm_gpio_to_channel(LS_PIN[phase]), level); // setze Duty (0..wrap)
}

// Schalte alle Ausgänge in sicheren Zustand (OFF)
void all_off(void)
{
    for (int i = 0; i < 3; ++i)
    {
        hs_drive_off(i);   // HS hochohmig (aus)
        ls_pwm_disable(i); // LS PWM aus
    }
}

// Busy-wait Deadtime (software Delay)
// Hinweis: sleep_us ist nicht hart echtzeit-deterministisch, aber ausreichend für einfache Deadtime.
static inline void deadtime_delay_us(uint32_t us)
{
    sleep_us(us); // blockiert die CPU für 'us' Mikrosekunden
}

// Prüfe ob E-Stop oder FAULT aktiv ist.
// is_fault_active gibt true zurück wenn einer der Sicherheitszustände gesetzt ist.
bool is_fault_active(void)
{
    if (!gpio_get(ESTOP_PIN)) // gpio_get liefert 0 wenn Pin LOW -> active LOW E-Stop gedrückt
        return true;
    if (gpio_get(FAULT_PIN)) // FAULT assumed active HIGH -> 1 bedeutet Fehler
        return true;
    return false; // kein Fehler
}

// -------------------- Kommutationslogik --------------------
// 6-Schritt Sequenz (trapezoidal). Jede Zeile {HS_phase, LS_phase}:
// z.B. {0,1} = HS Phase A on, LS Phase B PWM, Phase C floating.
const int COMMUTATION[6][2] = {
    {0, 1},
    {0, 2},
    {1, 2},
    {1, 0},
    {2, 0},
    {2, 1}};

// commutate_step führt eine Kommutationsstufe sicher aus.
// step: Index 0..5, pwm_level: Duty Wert (0..PWM_WRAP)
void commutate_step(int step, uint16_t pwm_level)
{
    static int current_hs = -1; // Merker für aktuell eingeschalteten HS
    static int current_ls = -1; // Merker für aktuell eingeschalteten LS

    int new_hs = COMMUTATION[step][0]; // Ziel HS Phase für diesen Schritt
    int new_ls = COMMUTATION[step][1]; // Ziel LS Phase für diesen Schritt

    // Sofort abschalten und zurück, falls ein Fehler aktiv ist
    if (is_fault_active())
    {
        all_off();
        current_hs = current_ls = -1; // Zustandsmerker zurücksetzen
        return;                       // verlasse Funktion ohne Umschalten
    }

    // 1) Deaktiviere aktuell aktive Low-Side (wenn vorhanden)
    if (current_ls != -1)
    {
        ls_pwm_disable(current_ls); // PWM aus
        current_ls = -1;            // Merker löschen
    }
    deadtime_delay_us(DEAD_TIME_US); // warte Deadtime nach LS-off

    // 2) Stelle sicher, dass alle HS außer dem neuen deaktiviert sind
    for (int ph = 0; ph < 3; ++ph)
    {
        if (ph != new_hs)
        {
            hs_drive_off(ph); // hochohmig setzen -> externe Pullup zieht Gate auf 5V
        }
    }
    deadtime_delay_us(DEAD_TIME_US); // weitere Deadtime

    // 3) Aktiviere die gewünschte HS (P-MOSFET ON)
    hs_drive_on(new_hs);
    current_hs = new_hs;
    deadtime_delay_us(DEAD_TIME_US); // nochmal Deadtime, damit HS stabil leitet

    // 4) Aktiviere die neue Low-Side PWM (N-MOSFET)
    ls_pwm_enable(new_ls, pwm_level);
    current_ls = new_ls;
}

// -------------------- Button Handling (Debounce + Repeat) --------------------
// Struktur zur Verwaltung des Debounce- und Repeat-Zustandes eines Tasters
typedef struct
{
    uint pin;                  // GPIO Pin Nummer
    bool last_stable_state;    // zuletzt stabil gemessener Zustand (true = offen / nicht gedrückt)
    uint32_t last_change_time; // Zeitpunkt der letzten Zustandsänderung (ms seit Boot)
    uint32_t last_event_time;  // Zeitpunkt, an dem zuletzt ein Event (Press/Repeat) ausgelöst wurde
} btn_t;

static btn_t btn_inc; // Taster "schneller"
static btn_t btn_dec; // Taster "langsamer"

// Initialisierung der Tasterpins und der btn_t Strukturen
void buttons_init(void)
{
    btn_inc.pin = BUTTON_INC_PIN;                                     // Pin setzen
    btn_inc.last_stable_state = true;                                 // default: nicht gedrückt (Pullup)
    btn_inc.last_change_time = to_ms_since_boot(get_absolute_time()); // initial Timestamp
    btn_inc.last_event_time = 0;                                      // noch kein Event

    btn_dec.pin = BUTTON_DEC_PIN;
    btn_dec.last_stable_state = true;
    btn_dec.last_change_time = to_ms_since_boot(get_absolute_time());
    btn_dec.last_event_time = 0;

    // GPIO Konfiguration: Input mit Pull-Up, da active LOW Taster
    gpio_init(btn_inc.pin);             // Pin initialisieren
    gpio_set_dir(btn_inc.pin, GPIO_IN); // als Input
    gpio_pull_up(btn_inc.pin);          // internen Pullup aktivieren (Pin = 1 wenn offen)

    gpio_init(btn_dec.pin);
    gpio_set_dir(btn_dec.pin, GPIO_IN);
    gpio_pull_up(btn_dec.pin);
}

// Prüft den Taster auf gedrückt / gehalten / repeat und gibt true zurück wenn ein Event produziert werden soll.
bool button_check_and_consume(btn_t *b)
{
    uint32_t now = to_ms_since_boot(get_absolute_time()); // aktuelle Zeit in ms seit Boot
    bool raw = gpio_get(b->pin);                          // lese aktuellen Pinzustand: true=High=nicht gedrückt, false=Low=gedrückt

    if (raw != b->last_stable_state)
    {
        // Zustand hat sich geändert -> beginne Debounce-Logik
        // Wenn initiale Änderung, setze last_change_time (oder korrigiere bei Überlauf)
        if ((int32_t)(now - b->last_change_time) < 0 || b->last_change_time == 0)
        {
            b->last_change_time = now;
        }
        // Wenn Zustand seit Debounce-Zeit stabil ist, akzeptiere neuen Zustand
        if ((now - b->last_change_time) >= BUTTON_DEBOUNCE_MS)
        {
            b->last_stable_state = raw; // neuer stabiler Zustand
            b->last_change_time = now;
            if (!raw)
            {
                // neu gedrückt (active low). Generiere sofort ein Event und setze last_event_time (für Repeat)
                b->last_event_time = now;
                return true; // Button-Event (Press)
            }
            return false; // Release -> kein Event erzeugen
        }
        else
        {
            // Noch innerhalb Debounce-Periode -> keine Aktion
            return false;
        }
    }
    else
    {
        // Zustand unverändert
        if (!b->last_stable_state)
        {
            // Button ist weiterhin gedrückt -> prüfen ob Repeat fällig ist
            if ((now - b->last_event_time) >= BUTTON_REPEAT_MS)
            {
                b->last_event_time = now; // Wiederholzeitpunkt aktualisieren
                return true;              // wiederholtes Event
            }
        }
        return false; // kein Event
    }
}

// -------------------- Init --------------------
// Initialisiert alle Pins, PWM-Slices und Buttons
void init_pins_and_pwm(void)
{
    stdio_init_all(); // Initialisiert USB/UART-stdio je nach Board/CMake Einstellung (für printf)

    // E-Stop Pin konfigurieren: Input mit Pull-Up (active LOW)
    gpio_init(ESTOP_PIN);
    gpio_set_dir(ESTOP_PIN, GPIO_IN);
    gpio_pull_up(ESTOP_PIN);

    // FAULT Pin konfigurieren: Input, hier Pull-Down angenommen (active HIGH)
    gpio_init(FAULT_PIN);
    gpio_set_dir(FAULT_PIN, GPIO_IN);
    gpio_pull_down(FAULT_PIN);

    // HS Pins initial als Input (hochohmig) damit externe Pullups die HS off halten
    for (int i = 0; i < 3; ++i)
    {
        gpio_init(HS_PIN[i]);
        gpio_set_dir(HS_PIN[i], GPIO_IN); // hochohmig
        gpio_disable_pulls(HS_PIN[i]);    // keine internen Pulls verwenden (nutze externe 5V Pullups)
    }

    // PWM Konfiguration für LS Pins
    for (int i = 0; i < 3; ++i)
    {
        gpio_set_function(LS_PIN[i], GPIO_FUNC_PWM);   // Funktion des Pins auf PWM setzen
        uint slice = pwm_gpio_to_slice_num(LS_PIN[i]); // Bestimme welches PWM-Slice dieser Pin verwendet
        pwm_set_wrap(slice, PWM_WRAP);                 // setze "wrap" (Auflösung) des PWM Generators

        // clock_get_hz liefert Systemclock-Frequenz (Hz) -> wir berechnen den Clock-Divider
        float clk = (float)clock_get_hz(clk_sys); // total system clock in Hz (z.B. 125000000)
        // Divider berechnen um PWM_FREQ zu erreichen: divider = clk / ((wrap+1) * PWM_FREQ)
        float divider = clk / ((float)(PWM_WRAP + 1) * PWM_FREQ);
        if (divider < 1.0f)
            divider = 1.0f;             // Divider darf nicht < 1 sein
        pwm_set_clkdiv(slice, divider); // setze Clock-Divider für den Slice

        // Kanal-Level initial 0 (kein PWM)
        pwm_set_chan_level(slice, pwm_gpio_to_channel(LS_PIN[i]), 0u);
        pwm_set_enabled(slice, true); // aktiviere den PWM-Slice (wichtig)
    }

    buttons_init(); // konfiguriere Button-Pins
}

// -------------------- Main --------------------
int main()
{
    init_pins_and_pwm(); // Hardware initialisieren
    all_off();           // alle Ausgänge in sicheren Zustand setzen

    printf("BLDC driver (buttons) started. step_time_ms=%u\n", step_time_ms);
    // Debug-Ausgabe, damit du beim Start parametrierte Werte siehst (z.B. über UART)

    // pwm_level = 80% DutyCycle initial; hier als Wert im Bereich 0..PWM_WRAP
    uint16_t pwm_level = (uint32_t)PWM_WRAP * 80 / 100; // 80% initial

    int step = 0; // aktueller Kommutationsschritt (0..5)
    while (true)
    {
        // Überprüfe während jeder Iteration auf Not-Aus / Fault
        if (is_fault_active())
        {
            printf("Fault/EStop active -> all off\n"); // Debug-Ausgabe
            all_off();                                 // sichere Abschaltung
            // blockiere, bis Fehler manuell gelöscht wird (z. B. Taster loslassen)
            while (is_fault_active())
                sleep_ms(100);
            printf("Fault cleared. Resuming.\n");
            all_off();    // nochmals sicherstellen
            sleep_ms(50); // kleines Delay zur Stabilisierung
        }

        // Führe eine Kommutationsstufe mit dem aktuellen PWM-Level aus
        commutate_step(step, pwm_level);

        // Warte insgesamt step_time_ms, prüfe aber regelmäßig Buttons (Polling),
        // damit Taster schnell reagieren (anstatt einen großen blocking sleep)
        uint32_t waited = 0;
        while (waited < step_time_ms)
        {
            sleep_ms(POLL_MS); // kurzer Sleep
            waited += POLL_MS;

            // Buttons prüfen und ggf. step_time_ms anpassen
            if (button_check_and_consume(&btn_inc))
            {
                // Increase speed => reduce step_time_ms (schneller)
                if (step_time_ms > STEP_TIME_MIN_MS + STEP_TIME_STEP_MS)
                    step_time_ms -= STEP_TIME_STEP_MS;
                else
                    step_time_ms = STEP_TIME_MIN_MS;
                printf("Speed UP -> step_time_ms=%u\n", step_time_ms);
            }
            if (button_check_and_consume(&btn_dec))
            {
                // Decrease speed => increase step_time_ms (langsamer)
                if (step_time_ms + STEP_TIME_STEP_MS < STEP_TIME_MAX_MS)
                    step_time_ms += STEP_TIME_STEP_MS;
                else
                    step_time_ms = STEP_TIME_MAX_MS;
                printf("Speed DOWN -> step_time_ms=%u\n", step_time_ms);
            }

            // Reagiere sofort, falls während Polling ein Fehler auftritt
            if (is_fault_active())
            {
                all_off();
                break; // Abbruch der Warte-Schleife, zurück zur äußeren Schleife
            }
        }

        // nächsten Kommutationsschritt wählen (zyklisch 0..5)
        step = (step + 1) % 6;
    }

    return 0; // wird nie erreicht, da while(true) endlos läuft
}