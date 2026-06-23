#include <WiFi.h>
#include <WiFiManager.h>
#include <FirebaseESP32.h> 
#include <Preferences.h> 

#include "DiabetesModel.h"

// --- FIREBASE CREDENTIALS ---
#define API_KEY "AIzaSyAwVLi7VjU0uGx5w5wvgFauCYiUiPaPQm0"
#define DATABASE_URL "diabetes-predictor-b8d76-default-rtdb.asia-southeast1.firebasedatabase.app"

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

DiabetesModel mlModel;
Preferences prefs;

void setup() {
  Serial.begin(115200);
  
  // ⏳ ESP32-S3 FIX: Wait for Windows to mount the USB Serial Monitor!
  delay(3000); 

  // 💾 1. OPEN PERMANENT MEMORY
  prefs.begin("ml_data", false);
  
  // ⚠️ WIPE MEMORY: Leave this for the VERY FIRST upload to force a clean download. 
  // ONCE YOU SEE "✅ Saved Permanently" IN THE SERIAL MONITOR, 
  // DELETE THIS LINE AND RE-UPLOAD THE CODE ONE LAST TIME!
  //prefs.clear(); 
  
  float saved_weights[10], saved_means[10], saved_stds[10], saved_bias;
  
  if (prefs.getBytesLength("weights") > 0) {
    Serial.println("💾 Loading ML Model from internal permanent memory...");
    prefs.getBytes("weights", saved_weights, sizeof(saved_weights));
    prefs.getBytes("means", saved_means, sizeof(saved_means));
    prefs.getBytes("stds", saved_stds, sizeof(saved_stds));
    saved_bias = prefs.getFloat("bias", 0.0);
    
    mlModel.setModelParameters(saved_weights, saved_bias, saved_means, saved_stds);
  } else {
    Serial.println("🧠 Memory empty. Needs a Firebase download.");
  }

  // 📶 2. CONNECT TO WIFI
  WiFiManager wm;
  Serial.println("Starting WiFi Config...");
  wm.setConfigPortalTimeout(180); 
  
  if (!wm.autoConnect("ESP32_Diabetes_Setup")) {
    Serial.println("WiFi failed to connect. Running in offline mode...");
  } else {
    Serial.println("\nWiFi Connected! Checking Firebase for Model Updates...");
    
    config.api_key = API_KEY;
    config.database_url = DATABASE_URL;
    config.signer.test_mode = true;
    Firebase.begin(&config, &auth);
    Firebase.reconnectWiFi(true);

    // ☁️ 3. DOWNLOAD NEW BRAIN FROM FIREBASE
    float cloud_weights[10], cloud_means[10], cloud_stds[10], cloud_bias;
    bool downloadSuccess = true;

    if (Firebase.getFloat(fbdo, "/app/model/bias")) cloud_bias = fbdo.floatData(); else downloadSuccess = false;

    for(int i = 0; i < 10; i++) {
      if (Firebase.getFloat(fbdo, "/app/model/weights/" + String(i))) cloud_weights[i] = fbdo.floatData(); else downloadSuccess = false;
      if (Firebase.getFloat(fbdo, "/app/model/means/" + String(i))) cloud_means[i] = fbdo.floatData(); else downloadSuccess = false;
      if (Firebase.getFloat(fbdo, "/app/model/stds/" + String(i))) cloud_stds[i] = fbdo.floatData(); else downloadSuccess = false;
    }

    // 💾 4. SAVE NEW BRAIN TO PERMANENT MEMORY
    if (downloadSuccess) {
      mlModel.setModelParameters(cloud_weights, cloud_bias, cloud_means, cloud_stds);
      
      prefs.putBytes("weights", cloud_weights, sizeof(cloud_weights));
      prefs.putBytes("means", cloud_means, sizeof(cloud_means));
      prefs.putBytes("stds", cloud_stds, sizeof(cloud_stds));
      prefs.putFloat("bias", cloud_bias);
      
      Serial.println("✅ New Model Downloaded and Saved Permanently!");
    } else {
      Serial.println("⚠️ Firebase download failed. Using older saved model if available.");
    }
  }
}

void loop() {
  if (WiFi.status() == WL_CONNECTED && Firebase.ready()) {
    if (Firebase.getBool(fbdo, "/app/trigger")) {
      if (fbdo.boolData() == true) {
        Serial.println("\n--- Predicting Diabetes ---");
        
        Firebase.getFloat(fbdo, "/app/inputs/age"); float age = fbdo.floatData();
        Firebase.getFloat(fbdo, "/app/inputs/bmi"); float bmi = fbdo.floatData();
        Firebase.getFloat(fbdo, "/app/inputs/hba1c"); float hba1c = fbdo.floatData();
        Firebase.getFloat(fbdo, "/app/inputs/glucose"); float glucose = fbdo.floatData();
        Firebase.getFloat(fbdo, "/app/inputs/gender"); float gender = fbdo.floatData();
        Firebase.getFloat(fbdo, "/app/inputs/smoking"); float smoking = fbdo.floatData();
        Firebase.getFloat(fbdo, "/app/inputs/hyper"); float hyper = fbdo.floatData();
        Firebase.getFloat(fbdo, "/app/inputs/heart"); float heart = fbdo.floatData();

        int result = mlModel.predict(age, bmi, hba1c, glucose, gender, smoking, hyper, heart);

        if (result == -1) {
          Serial.println("Error: Model has no brain yet! Connect to WiFi once to download.");
        } else {
          Serial.print("Result: ");
          Serial.println(result == 1 ? "HIGH RISK (Diabetic)" : "LOW RISK");
          Firebase.setInt(fbdo, "/app/result", result);
        }

        Firebase.setBool(fbdo, "/app/trigger", false);
      }
    }
  }
  delay(1000);
}