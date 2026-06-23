import pandas as pd
import requests
from sklearn.model_selection import train_test_split
from sklearn.preprocessing import StandardScaler
from sklearn.linear_model import LogisticRegression

print("STEP 1: TRAINING MODEL...")
df = pd.read_csv("diabetes_prediction_dataset.csv")

# Clean & Encode
df['gender'] = df['gender'].replace({'Other': 'Male'})
df['smoking_history'] = df['smoking_history'].replace({'not current': 'former', 'ever': 'former'})
df = pd.get_dummies(df, drop_first=True)

X = df.drop('diabetes', axis=1)
y = df['diabetes']

X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=0.2, random_state=42)

scaler = StandardScaler()
X_train_scaled = scaler.fit_transform(X_train)
X_test_scaled = scaler.transform(X_test)

model = LogisticRegression(max_iter=1000)
model.fit(X_train_scaled, y_train)
print(f"Accuracy: {model.score(X_test_scaled, y_test)}")

print("\nSTEP 2: UPLOADING TO FIREBASE...")
model_data = {
    "weights": model.coef_[0].tolist(),
    "bias": float(model.intercept_[0]),
    "means": scaler.mean_.tolist(),
    "stds": scaler.scale_.tolist()
}

firebase_url = "https://diabetes-predictor-b8d76-default-rtdb.asia-southeast1.firebasedatabase.app/app/model.json"

response = requests.put(firebase_url, json=model_data)

if response.status_code == 200:
    print("✅ SUCCESS! ML Model constants uploaded to Firebase.")
else:
    print("❌ ERROR uploading to Firebase:", response.text)
