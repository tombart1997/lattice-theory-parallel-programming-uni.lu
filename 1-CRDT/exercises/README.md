# Conflict-Free Replicated Data Types (1-CRDT)

* `1-CRDT/exercise`: contains the exercises (`.py`files).

## Prerequesities
```bash 
git clone https://github.com/ptal/lattice-theory-parallel-programming-uni.lu.git
cd lattice-theory-parallel-programming-uni.lu/1-CRDT/exercise 
conda env create -f environment.yml
conda activate lattice_gym
```
or
```bash
git clone https://github.com/ptal/lattice-theory-parallel-programming-uni.lu.git
cd lattice-theory-parallel-programming-uni.lu/1-CRDT/exercise 
python3 -m venv <your-env-name>
source <your-env-name>/bin/activate
pip install PyQt6
```

## How to use?

First, we launch server to collect all the clients information `(ip, port)`. After that, we send these clients information back to all other clients with their ID.
For example, `INFO:id, ip, port_number`. `id` means the index of the client in the group. `ip` in this exercise is going to be `localhost`, since we are only working on same machine. `port_number` represents the specific way to connect to other clients.

```bash
# launch server
python3 server.py

# launch 2 clients
python3 client.py # client 1
python3 client.py # client 2
```

**PS After launching 2 clients, the server can be terminated.**


## Exercise G-Counter CRDT

1. Implement **the G-Counter CRDT with 2 clients** as seen in class. You can use a simple gossip protocol between the clients triggered every 10 seconds through the socket interface.
2. Is the counter of the clients eventually consistent? Test your implementation with different scenario.
3. Extend the code to be more general **with arbitrary number of clients**. (Hint: you have to modify `server.py` and `client.py`)
