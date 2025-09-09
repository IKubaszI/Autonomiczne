from flask import Flask, request, jsonify
from datetime import datetime, timezone
from pymongo import MongoClient
import sqlite3
import traceback

app = Flask(__name__)
DB_PATH = "serwer/esp_data.db"


client = MongoClient("mongodb+srv://ESP:ESP@ESP.a0o4qzt.mongodb.net/ESP?retryWrites=true&w=majority")
db = client["ESP"]
collection = db["sensordatas"]


def init_db():
    conn = sqlite3.connect(DB_PATH)
    c = conn.cursor()
    c.execute("""
        CREATE TABLE IF NOT EXISTS SensorData (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            distance REAL,
            timestamp TEXT DEFAULT CURRENT_TIMESTAMP
        )
    """)
    conn.commit()
    conn.close()


@app.route('/api/data', methods=['POST'])
def add_sensor_data():
    try:
        data = request.get_json()
        distance = data.get("distance", 0)
        timestamp = datetime.now(timezone.utc).isoformat()

        #mongo insert
        collection.insert_one({
            "distance": distance,
            "timestamp": timestamp
        })

        # SQLite insert
        conn = sqlite3.connect(DB_PATH)
        c = conn.cursor()
        c.execute("INSERT INTO SensorData (distance, timestamp) VALUES (?, ?)", (distance, timestamp))
        conn.commit()
        conn.close()

        print('Dane zapisane:', data)
        return "Dane zapisane OK", 200
    except Exception as e:
        print('Błąd przy zapisie:', e)
        traceback.print_exc()
        return "Błąd zapisu", 500

@app.route('/api/all-data', methods=['GET'])
def get_all_data():
    try:
        # MongoDB read
        docs = collection.find().sort("timestamp", -1)
        data = []
        for doc in docs:
            try:
                dt = datetime.fromisoformat(doc["timestamp"])
                formatted_time = dt.strftime('%Y-%m-%d %H:%M:%S')
            except Exception:
                formatted_time = doc["timestamp"]
            data.append({
                "id": str(doc["_id"]),
                "distance": doc["distance"],
                "timestamp": formatted_time
            })
        return jsonify(data), 200
    except Exception as e:
        print('Błąd podczas pobierania danych:', e)
        traceback.print_exc()
        return "Błąd pobierania danych", 500

@app.route('/api/clear-data', methods=['GET'])
def clear_data():
    try:
        # MongoDB clear
        collection.delete_many({})
        # SQLite clear
        conn = sqlite3.connect(DB_PATH)
        c = conn.cursor()
        c.execute("DELETE FROM SensorData")
        conn.commit()
        conn.close()

        print('Dane usunięte z obu baz')
        return "Dane usunięte", 200
    except Exception as e:
        print('Błąd przy usuwaniu danych:', e)
        traceback.print_exc()
        return "Błąd usuwania danych", 500

@app.route('/api/test-data', methods=['GET'])
def add_test_data():
    try:
        test_entries = [
            {"distance": 123, "timestamp": datetime.now(timezone.utc).isoformat()},
            {"distance": 456, "timestamp": datetime.now(timezone.utc).isoformat()},
            {"distance": 789, "timestamp": datetime.now(timezone.utc).isoformat()}
        ]

        # MongoDB insert
        collection.insert_many(test_entries)

        # SQLite insert
        conn = sqlite3.connect(DB_PATH)
        c = conn.cursor()
        for entry in test_entries:
            c.execute("INSERT INTO SensorData (distance, timestamp) VALUES (?, ?)",
                      (entry["distance"], entry["timestamp"]))
        conn.commit()
        conn.close()

        print("Dodano dane testowe do obu baz")
        return "Dodano dane testowe", 200
    except Exception as e:
        print('Błąd przy dodawaniu danych testowych:', e)
        traceback.print_exc()
        return "Błąd dodawania danych testowych", 500


if __name__ == "__main__":
    init_db()
    app.run(host="0.0.0.0", port=3100, debug=True)
