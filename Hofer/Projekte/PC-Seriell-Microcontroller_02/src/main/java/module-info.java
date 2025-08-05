module com.example.testlauf_02 {
    requires javafx.controls;
    requires javafx.fxml;
    requires com.fazecast.jSerialComm;


    opens com.example.testlauf_02 to javafx.fxml;
    exports com.example.testlauf_02;
}