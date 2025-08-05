package com.example.pcseriellucschritt02;


import com.fazecast.jSerialComm.SerialPort;
import javafx.fxml.FXML;
import javafx.scene.control.Button;
import javafx.scene.control.TextField;

public class HelloController {

    @FXML private TextField portField;
    @FXML private Button connectButton;

    @FXML private TextField inputField;
    @FXML private Button sendButton;

    private SerialPort serialPort;

    @FXML
    public void initialize() {
        connectButton.setOnAction(e -> connectToPort());
        sendButton.setOnAction(e -> {
            String command = inputField.getText().trim();
            sendCommand(command);
        });
    }

    private void connectToPort() {
        String portName = portField.getText().trim();
        if (portName.isEmpty()) {
            System.err.println("Falscher Port");
            return;
        }

        serialPort = SerialPort.getCommPort(portName);
        if (serialPort.openPort()) {
            serialPort.setBaudRate(115200);
            serialPort.setComPortTimeouts(SerialPort.TIMEOUT_WRITE_BLOCKING, 0, 0);
            System.out.println("Vverbunden mit:" + portName);

            // GUI aktivieren
            inputField.setDisable(false);
            sendButton.setDisable(false);
        } else {
            System.err.println("Verbindungsfehler");
        }
    }

    private void sendCommand(String text) {
        if (serialPort != null && serialPort.isOpen()) {
            byte[] data = (text + "\n").getBytes();
            serialPort.writeBytes(data, data.length);
            System.out.println("Gesendet: " + text);
        }
    }
}


