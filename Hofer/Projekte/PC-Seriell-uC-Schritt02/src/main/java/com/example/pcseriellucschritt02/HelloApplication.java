/**
 * HelloApplication.java - JavaFX Programm für serielle Kommunikation (Schritt03)
 *
 * Aufgabenstellung:
 * - Verbindet sich über JSerialComm mit der seriellen Schnittstelle.
 * - Sendet Befehle (Strings) an den Microcontroller.
 * - Empfängt Nachrichten und zeigt sie in der JavaFX-Oberfläche an.
 */

package com.example.pcseriellucschritt02;

import com.fazecast.jSerialComm.SerialPort;
import javafx.application.Application;
import javafx.fxml.FXMLLoader;
import javafx.scene.Scene;
import javafx.stage.Stage;

public class HelloApplication extends Application {
    public static SerialPort serialPort;

    @Override
    public void start(Stage stage) throws Exception {
        FXMLLoader fxmlLoader = new FXMLLoader(HelloApplication.class.getResource("hello-view.fxml"));
        Scene scene = new Scene(fxmlLoader.load(), 400, 300);
        stage.setTitle("Serielle Kommunikation Schritt03");
        stage.setScene(scene);
        stage.show();

        // Serielle Schnittstelle öffnen
        SerialPort[] ports = SerialPort.getCommPorts();
        if (ports.length > 0) {
            serialPort = ports[0];
            serialPort.setBaudRate(9600);
            serialPort.openPort();
            System.out.println("Serielle Verbindung geöffnet: " + serialPort.getSystemPortName());
        } else {
            System.out.println("Keine seriellen Ports gefunden!");
        }
    }

    @Override
    public void stop() throws Exception {
        if (serialPort != null && serialPort.isOpen()) {
            serialPort.closePort();
            System.out.println("Serielle Verbindung geschlossen.");
        }
    }

    public static void main(String[] args) {
        launch();
    }
}