/*
 * HelloApplication.java - Schritt03 Easy
 *
 * JavaFX Programm, das serielle Befehle an den Microcontroller sendet.
 * In dieser einfachen Version werden nur Kommandos gesendet.
 * Empfangen und GUI-Aktualisierung kann leicht erweitert werden.
 */

import javafx.application.Application;
import javafx.stage.Stage;
import javafx.scene.Scene;
import javafx.scene.control.*;
import javafx.scene.layout.VBox;
import com.fazecast.jSerialComm.*;

import java.io.OutputStream;

public class HelloApplication extends Application {

    private SerialPort serialPort;
    private OutputStream output;

    @Override
    public void start(Stage stage) {
        VBox root = new VBox(10);
        TextField inputField = new TextField();
        Button sendButton = new Button("Senden");
        Label status = new Label("Nicht verbunden");

        sendButton.setOnAction(e -> sendCommand(inputField.getText()));

        root.getChildren().addAll(inputField, sendButton, status);
        stage.setScene(new Scene(root, 300, 200));
        stage.setTitle("Serielle Steuerung");
        stage.show();

        connectSerial(status);
    }

    private void connectSerial(Label status) {
        SerialPort[] ports = SerialPort.getCommPorts();
        if (ports.length > 0) {
            serialPort = ports[0];
            serialPort.setBaudRate(9600);
            if (serialPort.openPort()) {
                output = serialPort.getOutputStream();
                status.setText("Verbunden mit: " + serialPort.getSystemPortName());
            } else {
                status.setText("Konnte Port nicht öffnen");
            }
        } else {
            status.setText("Kein COM-Port gefunden");
        }
    }

    private void sendCommand(String cmd) {
        try {
            if (output != null && cmd != null && !cmd.isEmpty()) {
                output.write((cmd + "\n").getBytes());
                output.flush();
            }
        } catch (Exception ex) {
            ex.printStackTrace();
        }
    }

    public static void main(String[] args) {
        launch();
    }
}
