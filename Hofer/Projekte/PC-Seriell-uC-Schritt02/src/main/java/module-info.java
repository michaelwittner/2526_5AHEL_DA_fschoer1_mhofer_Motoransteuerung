module com.example.pcseriellucschritt02 {
    requires javafx.controls;
    requires javafx.fxml;
    requires com.fazecast.jSerialComm;


    opens com.example.pcseriellucschritt02 to javafx.fxml;
    exports com.example.pcseriellucschritt02;
}