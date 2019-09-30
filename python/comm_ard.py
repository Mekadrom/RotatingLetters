import serial

class ard_connect():
    def __init__(self, parent):
        self.parent = parent
        self.startMarker = 60 #utf-8 for '<'
        self.endMarker = 62   #utf-8 for '>'
        print("ard created")

    def connect(self, port):
        try:
            self.ser = serial.Serial(port, 115200)
            self.waitForArduino()
            self.parent.is_connected = True
            return True
        except:
            print("Not able to connect on this port")
            return False

    def waitForArduino(self): # waits for arduino to send "<connected>"
        msg = ""
        while msg.find("connected") == -1:
            while self.ser.inWaiting() == 0:
                pass

            msg = self.recvFromArduino()
            print(msg)

    def recvFromArduino(self):
        message_received = ""
        x = "z"
        byteCount = -1

        while ord(x) != self.startMarker:
            x = self.ser.read()

        while ord(x) != self.endMarker:
            if ord(x) != self.startMarker:
                message_received = message_received + x.decode("utf-8")
                byteCount += 1
            x = self.ser.read()

        return (message_received)

    def runTest(self, message_to_send):
        self.sendToArduino(message_to_send)
        self.parent.update_LCD_display()

    def sendToArduino(self, sendStr):
        self.ser.write(sendStr.encode('utf-8'))
        