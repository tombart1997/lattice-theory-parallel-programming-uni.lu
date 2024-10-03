import sys
from typing import Tuple
from PyQt6.QtWidgets import QApplication, QWidget, QLabel, QPushButton, QVBoxLayout
import random
import socket
import threading
import time


class MyApp(QWidget):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("Lattice Gym")
        self.resize(300, 300)  # width, height
        layout = QVBoxLayout()
        self.setLayout(layout)

        # socket parameters
        self.SERVER_PORT: int = 1234
        self.CLIENT_PORT: int = random.randint(5000, 9000)
        self.SERVER_NAME: str = "localhost"
        self.SERVER_ADDRESS: Tuple[str, int] = (self.SERVER_NAME, self.SERVER_PORT)
        self.CLIENT_ADDRESS: Tuple[str, int] = (self.SERVER_NAME, self.CLIENT_PORT)
        self.DATA_FORMAT: str = "utf-8"

        self.client: socket.socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.client.bind(self.CLIENT_ADDRESS)

        # widgets
        self.count_label = QLabel()
        self.count_label.setText("Hello, Welcome to lattice gym")
        self.debug_label = QLabel()
        self.debug_label.setText("State Vector Information")
        self.push_button = QPushButton()
        self.push_button.setText("Click me : )")

        layout.addWidget(self.count_label)
        layout.addWidget(self.debug_label)
        layout.addWidget(self.push_button)

        # initial value, after connection, it is going to be either 0 or 1.
        self.node_id: int = -1
        self.state_vector: list = [0, 0]
        self.push_button.clicked.connect(self.increment)
        self.start_CRDT: bool = False
        self.dest_address: Tuple[str,int]

    def request_value(self):
        # user interface function
        self.count_label.setText(f"Current value: {self._value()}")
        self.debug_label.setText(f"{self.state_vector[0]},{self.state_vector[1]}")

    def increment(self):
        # user interface function
        self._update()

    def _update(self):
        # TODO
        assert self.node_id != -1
        pass

    def _merge(self, other_state_vector: list):
        # TODO
        pass

    def _value(self):
        # TODO
        pass

    def receive(self):
        while True:
            try:
                message, _ = self.client.recvfrom(1024)
                message = message.decode(self.DATA_FORMAT)
                self.debug_label.setText(message)

                # only for getting another client connection information
                if message.startswith("INFO:"):
                    str_node_id, connection, port = message.split(":")[1].split(", ")
                    self.node_id = int(str_node_id)
                    self.dest_address = (connection, int(port))
                    print("Got another client's information")
                # TODO
                # receive the "state_vector" from another client
                
            except Exception as e:
                self.debug_label.setText(e)

    def start(self):
        # send connection information to the server
        self.client.sendto(f"SIGNUP_TAG:{self.CLIENT_PORT}".encode(self.DATA_FORMAT), self.SERVER_ADDRESS)
        while not self.start_CRDT:
            if self.node_id != -1:
                self.start_CRDT = True

        print("start CRDT")
        # TODO:
        # connect with another client
        # send the "state_vector" to another client every 10 seconds



if __name__ == "__main__":
    app = QApplication(sys.argv)
    window = MyApp()
    t1 = threading.Thread(target=window.start, daemon=True)
    t2 = threading.Thread(target=window.receive, daemon=True)
    t1.start()
    t2.start()
    window.show()
    app.exec()
