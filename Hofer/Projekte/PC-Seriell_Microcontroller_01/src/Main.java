import com.fazecast.jSerialComm.SerialPort;
import java.util.Scanner;

public class Main {

    public static void main(String[] args) throws Exception {
        SerialPort[] ports = SerialPort.getCommPorts();
        System.out.println("Ports:");
        for (int i = 0; i < ports.length; i++) {
            System.out.println(i + ": " + ports[i].getSystemPortName());
        }

        Scanner scanner = new Scanner(System.in);
        System.out.print("Portnummer: ");
        int portIndex = scanner.nextInt();
        SerialPort comPort = ports[portIndex];

        comPort.setBaudRate(115200);
        if (!comPort.openPort()) {
            System.out.println("Fehler bei Ports");
            return;
        }

        System.out.println("Verbunden\n");

        scanner.nextLine();
        while (true) {
            System.out.print("> ");
            String input = scanner.nextLine().trim();
            if (input.equals("exit")) break;
            if (input.equals("on") || input.equals("off")) {
                comPort.getOutputStream().write((input + "\n").getBytes());
                comPort.getOutputStream().flush();
            } else {
                System.out.println("Fehler");
            }
        }

        comPort.closePort();
        System.out.println("Verbindung Ende:");
    }
}