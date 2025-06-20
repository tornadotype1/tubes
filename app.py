from flask import Flask, render_template, request, jsonify
from datetime import datetime

app = Flask(__name__)

# Variabel global untuk menyimpan data dari ESP32
current_bpm = 0
show_bpm = True

@app.route('/')
def index():
    return render_template("index.html", bpm=current_bpm, show_bpm=show_bpm)

@app.route('/time')
def get_time():
    now = datetime.now()
    current_time = now.strftime("%H:%M:%S")
    return jsonify(time=current_time, bpm=current_bpm, show_bpm=show_bpm)

@app.route('/data', methods=['POST'])
def receive_data():
    global current_bpm, show_bpm
    data = request.get_json()
    print("Data diterima:", data)
    current_bpm = data.get("bpm", 0)
    show_bpm = data.get("show_bpm", True)
    return jsonify(status="ok")

if __name__ == '__main__':
    app.run(host="0.0.0.0", port=5000, debug=True)
