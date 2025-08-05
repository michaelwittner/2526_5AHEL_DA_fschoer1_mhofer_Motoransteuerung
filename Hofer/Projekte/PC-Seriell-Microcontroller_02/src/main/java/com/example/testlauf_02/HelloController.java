package com.example.testlauf_02;

import com.fazecast.jSerialComm.SerialPort;
import javafx.fxml.FXML;
import javafx.scene.control.Label;
import javafx.scene.control.Slider;

public class HelloController {

    @FXML
    private Slider brightnessSlider;
    @FXML
    private Label valueLabel;
    private SerialPort serialPort;

    @FXML
    public void initialize() {
        serialPort = findPicoPort();
        if (serialPort != null && serialPort.openPort()) {
            serialPort.setBaudRate(115200);
            serialPort.setComPortTimeouts(SerialPort.TIMEOUT_WRITE_BLOCKING, 0, 0);
            System.out.println("Conn mit: " + serialPort.getSystemPortName());
        } else {
            System.err.println("Kein Pico");
        }

        brightnessSlider.valueProperty().addListener((obs, oldVal, newVal) -> {
            int value = newVal.intValue();
            valueLabel.setText("Wert: " + value);
            sendValueToPico(value);
        });
    }

    private void sendValueToPico(int value) {
        if (serialPort != null && serialPort.isOpen()) {
            byte[] data = {(byte) value};
            serialPort.writeBytes(data, 1);
        }
    }

    private SerialPort findPicoPort() {
        for (SerialPort port : SerialPort.getCommPorts()) {
            String desc = port.getDescriptivePortName().toLowerCase();
            if (desc.contains("pico") || desc.contains("usb") || port.getSystemPortName().toLowerCase().contains("ttyacm")) {
                return port;
            }
        }
        return null;
    }
}
