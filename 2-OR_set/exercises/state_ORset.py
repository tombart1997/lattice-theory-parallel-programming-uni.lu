import sys
from typing import Tuple, List, Set
from PyQt6.QtWidgets import QApplication, QWidget, QLabel, QPushButton, QVBoxLayout, QLineEdit
import random
import socket
import threading
import time
from functools import partial


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
        self.DISCONNECTION_INFO: str = "!DISCONNECT"

        self.client: socket.socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.client.bind(self.CLIENT_ADDRESS)

        # widgets
        self.state_label = QLabel()
        self.state_label.setText("Current State: ")
        self.info_label = QLabel()
        self.info_label.setText("Information: ")

        self.add_text = QLineEdit()
        self.add_text.setPlaceholderText("Enter the element to add")
        self.add_button = QPushButton()
        self.add_button.setText("Add")
        self.remove_text = QLineEdit()
        self.remove_text.setPlaceholderText("Enter the element to remove") 
        self.remove_button = QPushButton()
        self.remove_button.setText("Remove")
        self.lookup_text = QLineEdit()
        self.lookup_text.setPlaceholderText("Enter the element to lookup")
        self.lookup_button = QPushButton()
        self.lookup_button.setText("Lookup")

        layout.addWidget(self.state_label)
        layout.addWidget(self.info_label)

        layout.addWidget(self.add_text)
        layout.addWidget(self.add_button)
        layout.addWidget(self.remove_text)
        layout.addWidget(self.remove_button)
        layout.addWidget(self.lookup_text)
        layout.addWidget(self.lookup_button)

        # initial value
        self.node_id: int = -1
        #TODO:
        

        #
        self.start_CRDT: bool = False
        self.dest_address: List[Tuple[str,int]] = []
                
        self.add_button.clicked.connect(partial(self.update, isAdd=True))
        self.remove_button.clicked.connect(partial(self.update, isAdd=False)) 
        self.lookup_button.clicked.connect(self.lookup)


    def request_value(self):
        # user interface function
        #TODO:
        self.state_label.setText(f"Current State: xxxxx")

    def update(self, isAdd: bool):
        # user interface function
        #TODO:
        
        self.request_value()
        
    def lookup(self):
        # user interface function
        # TODO:
        pass

    def _add(self):
        # CRDT implementation
        add_value: str = self.add_text.text().strip()
        if add_value != "Enter the element to add":
            #TODO:
            pass

    def _remove(self):
        # CRDT implementation
        remove_value: str = self.remove_text.text().strip()
        if remove_value != "Enter the element to remove":
            #TODO: 
            pass
    
    def _lookup(self, lookup_value: str):
        # CRDT implementation
        # TODO:
        pass

    def _merge(self, other_A: set, other_T: set):
        # called asynchronously
        # TODO:
        pass

    def _value(self):
        # CRDT implementation -> user interface : value
        # TODO:
        pass

    def receive(self):
        # receive "state_vector" from another client
        # call merge function
        while True:
            try:
                message, _ = self.client.recvfrom(1024)
                message = message.decode(self.DATA_FORMAT)
                self.info_label.setText(message)
                if message.startswith("INFO:"):
                    str_node_id, connection, port = message.split(":")[1].split(", ")
                    self.node_id = int(str_node_id)
                    self.dest_address.append((connection, int(port)))
                    print("Got another client's information")
                elif len(message) > 0 and "OR-set A:" in message and "OR-set T:" in message:
                    # TODO:
                
                    # update count label
                    self.request_value()
            except Exception as e:
                self.info_label.setText(e)

    def start(self):
        # send message to the server
        self.client.sendto(f"SIGNUP_TAG:{self.CLIENT_PORT}".encode(self.DATA_FORMAT), self.SERVER_ADDRESS)
        while not self.start_CRDT:
            if self.node_id != -1:
                self.start_CRDT = True

        # connect with another client
        # send the "state_vector" to another client every 10 seconds
        print("start CRDT")
        while True:
            time.sleep(5)
            # TODO:
                

if __name__ == "__main__":
    app = QApplication(sys.argv)
    window = MyApp()
    t1 = threading.Thread(target=window.start, daemon=True)
    t2 = threading.Thread(target=window.receive, daemon=True)
    t1.start()
    t2.start()
    window.show()
    app.exec()
