#ifndef DIABETES_MODEL_H
#define DIABETES_MODEL_H

class DiabetesModel {
  private:
    float weights[10];
    float means[10];
    float stds[10];
    float bias;
    bool isReady = false; 

  public:
    DiabetesModel() {}

    void setModelParameters(float* w, float b, float* m, float* s) {
      for(int i = 0; i < 10; i++) {
        weights[i] = w[i];
        means[i] = m[i];
        stds[i] = s[i];
      }
      bias = b;
      isReady = true; 
    }

    int predict(float age, float bmi, float hba1c, float glucose, float gender, float smoking, float hyper, float heart) {
      if (!isReady) return -1; 
      
      float inputs[10] = {
        age, hyper, heart, bmi, hba1c, glucose,
        gender,                  
        (float)(smoking == 2),   
        (float)(smoking == 1),   
        (float)(smoking == 0)    
      };

      float score = bias;
      
      Serial.println("\n--- 🧠 ML Math Debug (X-Ray) ---");
      Serial.print("Loaded Bias: "); Serial.println(bias);
      
      for (int i = 0; i < 10; i++) {
        // Prevent Divide by Zero if memory corrupted!
        if (stds[i] == 0.0) stds[i] = 1.0; 

        float scaled = (inputs[i] - means[i]) / stds[i];
        float term = scaled * weights[i];
        score += term;
        
        Serial.printf("Feat %d | In:%.1f | Mean:%.1f | Std:%.1f | W:%.3f | Term:%.3f\n", 
                      i, inputs[i], means[i], stds[i], weights[i], term);
      }

      Serial.print("FINAL SCORE (z): "); Serial.println(score);
      Serial.println("--------------------------------");

      return (score > 0) ? 1 : 0;
    }
};

#endif