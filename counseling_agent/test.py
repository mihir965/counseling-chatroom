from flask import Flask, request, jsonify
import socket
import threading
import time
import os
from openai import OpenAI

app = Flask(__name__)

HOST = '127.0.0.1'
PORT = 8080

# Initialize OpenAI client (v1.x syntax)
client = OpenAI(api_key=os.environ.get("OPENAI_API_KEY"))

COUNSELOR_SYSTEM_PROMPT = """You are an empathetic and professional couples counselor. Your role is to:

1. Listen carefully to both partners' perspectives
2. Acknowledge each person's feelings without taking sides
3. Help identify underlying issues and communication patterns
4. Ask clarifying questions when needed
5. Suggest constructive ways to address conflicts
6. Maintain a warm, non-judgmental tone
7. Keep responses concise (2-4 sentences) to facilitate ongoing dialogue

Remember: Your goal is to help both partners feel heard and guide them toward mutual understanding, not to solve their problems for them."""


@app.route("/get_agent")
def get_agent():
    room_name = request.args.get("room")
    uuid = request.args.get("uuid")
    agent_name = f"ai_agent_{room_name}"
    if not room_name:
        return "Missing room_name", 400

    def connect_to_c_server():
        print("Trying to connect to c server")
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            try:
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
                        username = f"^ai_agent_{room_name}|{uuid}$"
                        s.sendall(username.encode('utf-8'))
                        query = f"@JOIN{room_name}#"
                        time.sleep(1.5)
                        s.sendall(query.encode('utf-8'))
                        just_joined = True
                        continue

                    if just_joined:
                        print(f"[AGENT-{room_name}] AI agent ready")
                        just_joined = False

            except ConnectionRefusedError:
                print(f"Connection refused by the server at {HOST}:{PORT}")
            except Exception as e:
                print(f"An error occurred: {e}")
            finally:
                s.close()

    threading.Thread(target=connect_to_c_server, daemon=True).start()
    data = {"agent_name": agent_name}
    return data, 200


@app.route("/get_counselor_response", methods=["POST"])
def get_counselor_response():
    try:
        data = request.json

        if not data or "messages" not in data:
            return jsonify({"error": "Missing messages in request"}), 400

        messages = data["messages"]
        room_name = data.get("room_name", "unknown")

        print(f"[COUNSELOR] Processing {
              len(messages)} messages for room: {room_name}")

        # Build OpenAI messages format
        openai_messages = [
            {"role": "system", "content": COUNSELOR_SYSTEM_PROMPT}
        ]

        # Add conversation history
        for msg in messages:
            username = msg.get("username", "Unknown")
            content = msg.get("content", "")
            formatted_content = f"{username}: {content}"
            openai_messages.append({
                "role": "user",
                "content": formatted_content
            })

        # Call OpenAI API (v1.x syntax)
        print("[COUNSELOR] Calling OpenAI API...")
        completion = client.chat.completions.create(
            model="gpt-3.5-turbo",
            messages=openai_messages,
            max_tokens=200,
            temperature=0.7
        )

        ai_response = completion.choices[0].message.content
        print(f"[COUNSELOR] AI Response: {ai_response}")

        return jsonify({"response": ai_response}), 200

    except Exception as e:
        print(f"[COUNSELOR] Error: {e}")
        import traceback
        traceback.print_exc()
        return jsonify({"error": str(e)}), 500


if __name__ == "__main__":
    if not os.environ.get("OPENAI_API_KEY"):
        print("WARNING: OPENAI_API_KEY environment variable not set!")
        print("Set it with: export OPENAI_API_KEY='your-key-here'")

    app.run(debug=True, port=5000)
