import sys
from PyQt5.QtWidgets import QApplication, QWidget
from PyQt5.QtGui import QIcon, QPixmap, QImage
from PyQt5.uic import loadUi
import opr
import comm_ard
import random
import pickle
import numpy as np
import cv2

class App(QWidget):

    def __init__(self):
        super().__init__()

        self.ui = loadUi('tracking.ui', self)

        self.LCameraID = 0
        self.RCameraID = 1

        self.rec = True
        
        self.capL = cv2.VideoCapture(self.LCameraID)
        self.capR = cv2.VideoCapture(self.RCameraID)

        self.capL.set(3, 1920)
        self.capL.set(4, 1080)
        
        self.capR.set(3, 1920)
        self.capR.set(4, 1080)

        self.f = 100.0 # focal length in inches, have to calculate this with the camera we get
        self.j = 4.0   # distance between cameras in inches; have to measure and/or plan for this
        self.w = 3.0   # distance between cameras and servos in inches; have to measure and/or plan for this
        
        self.xL = 0.0
        self.xR = 0.0
        
        self.calculated_dist = sys.maxsize # distance from cameras in inches
        self.calculated_angle = 90.0       # angle to object from center of cameras

        self.is_connected = False

        self.face_detected = False
        self.max_empty_frame = 50
        self.empty_frame_number = self.max_empty_frame 

        self.ard = comm_ard.ard_connect(self)
        self.initUI()

    def initUI(self):     #UI related stuff
        self.setWindowTitle('Face Tracker')             # set window title
        self.labelL = self.ui.labelL                    # set label (it will be used to display the captured images) left camera
        self.labelR = self.ui.labelR                    # set label (it will be used to display the captured images) right camera
        
        self.QuitButton = self.ui.QuitButton            # set quit button
        self.PauseButton = self.ui.PauseButton          # set pause button
        
        self.Dist_LCD = self.ui.Dist_LCD                # and so on...
        self.Angle_LCD = self.ui.Angle_LCD              # ...
        
        self.ConnectButton = self.ui.ConnectButton
        self.COMlineEdit = self.ui.COMlineEdit
        self.COMConnectLabel = self.ui.COMConnectLabel
        
        self.UpdateButton = self.ui.UpdateButton
        self.LCameraIDEdit = self.ui.LCameraIDEdit
        self.RCameraIDEdit = self.ui.RCameraIDEdit

        self.QuitButton.clicked.connect(self.quit)
        self.PauseButton.clicked.connect(self.toggle_recording)
        self.ConnectButton.clicked.connect(self.connect)
        self.UpdateButton.clicked.connect(self.update_attrs)

        self.load_init_file()
        self.update_attrs()

        self.record()  #start recording

    def load_init_file(self):
        #this method will allow to reload the latest values entered in text boxes even after closing the software
        try:         #try to open init file if existing
            with open('init.pkl', 'rb') as init_file:
                var = pickle.load(init_file)  #load all variable and update text boxes
                self.COMlineEdit.setText(var[0])
                self.LCameraIDEdit.setText(str(var[1]))
                self.RCameraIDEdit.setText(str(var[2]))
            print(var)
        except:
            pass

    def save_init_file(self):
        init_settings = [ self.COMlineEdit.text(), self.LCameraID, self.LCameraID ]
        with open('init.pkl', 'wb') as init_file:
            pickle.dump(init_settings, init_file)


    def connect(self):    #set COM port from text box if arduino not already connected
        if(not self.is_connected):
            port = self.COMlineEdit.text()
            if (self.ard.connect(port)):    #set port label message
                self.COMConnectLabel.setText("..................... Connected to port : " + port + " ......................")
            else:
                self.COMConnectLabel.setText(".................... Cant connect to port : " + port + " .....................")

    def update_attrs(self):  #update variables from text boxes
        try:
            self.capL.release()
            self.capR.release()
            self.LCameraID = int(self.LCameraIDEdit.text())
            self.RCameraID = int(self.RCameraIDEdit.text())
            self.capL = cv2.VideoCapture(self.LCameraID)
            self.capR = cv2.VideoCapture(self.RCameraID)
            
            self.save_init_file()
            print("values updated")
        except:
            print("can't update values")

    def update_LCD_display(self):
        self.Dist_LCD.display(self.calculated_dist)
        self.Angle_LCD.display(self.calculated_angle)

    def quit(self):
        print('Quit')
        self.rec = False
        sys.exit()

    def closeEvent(self, event):
        self.quit()

    def toggle_recording(self):
        if(self.rec):
            self.rec = False                   #stop recording
            self.PauseButton.setText("Resume") #change pause button text
        else:
            self.rec = True
            self.PauseButton.setText("Pause")
            self.record()

    def record(self):  #video recording
        while(self.rec):
            retL, imgL = self.capL.read() #CAPTURE IMAGE
            retR, imgR = self.capR.read() #CAPTURE IMAGE

            if(self.is_connected):
                processed_imgL = self.image_process(imgL)
                processed_imgR = self.image_process(imgR)
            else:
                processed_imgL = [ imgL, 0 ]
                processed_imgR = [ imgR, 0 ]

            self.update_GUI(processed_imgL[0], processed_imgR[0])
            
            self.xL = processed_imgL[1]
            self.xR = processed_imgR[1]
            
            cv2.waitKey(0)

            self.send_face_detected_and_data()

            if (not self.rec):
                break

    def update_GUI(self, openCV_imgL, openCV_imgR):
        try:
            openCV_imgL = cv2.resize(openCV_imgL, (480, 540))
            
            heightL, widthL, channelL = openCV_imgL.shape
            
            bytesPerLineL = 3 * widthL
            
            qImgL = QImage(openCV_imgL.data, widthL, heightL, bytesPerLineL, QImage.Format_RGB888).rgbSwapped()

            pixmapL = QPixmap(qImgL)
            
            self.labelL.setPixmap(pixmapL)

        except:
            self.labelL.setText("check camera ID")
            
        try:
            openCV_imgR = cv2.resize(openCV_imgR, (480, 540))
            
            heightR, widthR, channelR = openCV_imgR.shape
            
            bytesPerLineR = 3 * widthR
            
            qImgR = QImage(openCV_imgR.data, widthR, heightR, bytesPerLineR, QImage.Format_RGB888).rgbSwapped()

            pixmapR = QPixmap(qImgR)
            
            self.labelR.setPixmap(pixmapR)
        except:
            self.labelR.setText("check camera ID")
        self.show()

    def send_face_detected_and_data(self): # used to be move_servos
        self.calculate()
        if (self.is_connected):
            face_exists = ""
            if (self.face_detected):
                face_exists = "Y"
            else:
                face_exists = "N"
            self.ard.runTest(face_exists)
            self.ard.runTest(calculated_dist + "," + calculated_angle)

    def calculate(self):
        if not self.xL == self.xR:
            self.calculated_dist =  self.j * self.f / (self.xR - self.xL)
        else:
            self.calculated_dist = sys.maxsize
        
        if not self.xL == 0:
            thetaL = np.arctan(self.f / self.xL)
        else:
            thetaL = 90
            
        if not self.xR == 0:
            thetaR = np.arctan(self.f / self.xR)
        else:
            thetaR = 90
            
        self.calculated_angle = (thetaL + thetaR) / 2.0
        self.calculated_angle = np.arctan(((self.calculated_dist) / (self.calculated_dist - self.w)) * np.tan(self.calculated_angle))

    def image_process(self, img):
        processed_img = opr.find_face(img)

        if(processed_img[0]):             #if face found
            self.face_detected = True
            self.empty_frame_number = self.max_empty_frame  #reset empty frame count
            return [ processed_img[1], processed_img[2] ]
        else:
            self.face_detected = False
            if(self.empty_frame_number> 0):
                self.empty_frame_number -= 1  #decrease frame count until it equal 0
            return [ img, 0 ]

if __name__ == '__main__':
    app = QApplication(sys.argv)
    ex = App()
    app.exec_()
    #sys.exit(app.exec_())
