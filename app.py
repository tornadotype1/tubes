# app.py - Flask application with OLED control
from flask import Flask, render_template, request, jsonify
from datetime import datetime

app = Flask(__name__)

# Global variables to store sensor data
current_bpm = 0
last_update = None
oled_on = True

@app.route('/')
def index():
    return render_template("index.html", 
                           bpm=current_bpm,
                           last_update=last_update,
                           oled_on=oled_on)

@app.route('/api/bpm', methods=['POST'])
def update_bpm():
    global current_bpm, last_update
    data = request.json
    current_bpm = data.get('bpm', 0)
    last_update = datetime.now().strftime("%H:%M:%S")
    return jsonify({'status': 'success'})


@app.route('/control/oled', methods=['POST'])
def control_oled():
    global oled_on
    data = request.json
    if data is None or 'oled_on' not in data:
        return jsonify({'status': 'error', 'message': 'Missing oled_on field'}), 400
    oled_on = data['oled_on']
    return jsonify({'status': 'updated', 'oled_on': oled_on})


@app.route('/status/oled', methods=['GET'])
def get_oled_status():
    return jsonify({'oled_on': oled_on})

# Tambahkan ini di app.py
@app.route('/api/bpm', methods=['GET'])
def get_bpm():
    global current_bpm, last_update
    return jsonify({'bpm': current_bpm, 'last_update': last_update})

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000, debug=True)
