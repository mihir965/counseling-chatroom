from flask import Flask, request
import socket
import threading
import time

app = Flask(__name__)

HOST = '127.0.0.1'
PORT = 8080


@app.route("/get_agent")
def get_agent():
    room_name = request.args.get("room")
    agent_name = f"ai_agent_{room_name}"
    if not room_name:
        return "Missing room_name", 400

    def connect_to_c_server():
        print("Trying to connect to c server")
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            try:
                # Connect to the server
                s.connect((HOST, PORT))
                print(f"[AGENT-{room_name}] Connected to {HOST}:{PORT}")
                just_joined = False

                while True:
                    data = s.recv(1024)
                    if not data:
                        continue
                    msg = data.decode('utf-8').strip()
                    print(f"[AGENT-{room_name}] Received: {msg}")

                    if (msg == "Please enter your username:"):
                        print(f"[AGENT-{room_name}] Sending username...")
                        username = f"^ai_agent_{room_name}$"
                        s.sendall(username.encode('utf-8'))
                        query = f"@JOIN{room_name}#"
                        time.sleep(1.5)
                        s.sendall(query.encode('utf-8'))
                        just_joined = True
                        continue
                    if just_joined:
                        print(
                            f"[AGENT-{room_name}] Sending initial test message")
                        s.sendall(b"^this is a test$")
                        just_joined = False

            except ConnectionRefusedError:
                print(f"Connection refused by the server at {HOST}:{PORT}")
            except Exception as e:
                print(f"An error occured: {e}")
            finally:
                s.close()

    # Run the connection logic on a separate thread since the connect call is blocking and the c server is already blocking with the curl_easy function
    threading.Thread(target=connect_to_c_server, daemon=True).start()
    data = {
        "agent_name": agent_name
    }
    return data, 200


if __name__ == "__main__":
    app.run(debug=True)
