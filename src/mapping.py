import pandas as pd
import numpy as np
from sklearn.model_selection import train_test_split
from sklearn.preprocessing import PolynomialFeatures
from sklearn.linear_model import LinearRegression
import matplotlib.pyplot as plt

# Load the CSV file
file_path = 'src/5500.0_sensor_data.csv'

data = pd.read_csv(file_path)

# # Filter out rows where '|Diff|' is greater than 500
# print(len(data))
# data = data[data['|Diff|'] <= 3000]
# print(len(data))

# Filter out rows where MQ-4 > 10000
print(len(data))
data = data[data['Nano'] <= 5000]
print(len(data))

# Filter out rows where 'Nano' column is 0 and diff >= 500
print(len(data))
# data = data[(data['Nano'] != 0) | (data['|Diff|'] <= 100)]
data = data[(data['MQ-4'] != 0)]
data = data[(data['Nano'] != 0)]
print(len(data))

# Prepare the data
X = data['AO'].values.reshape(-1, 1)
y = data['Nano'].values

# Split the data into training and test sets
X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=0.2, random_state=42)

# Create polynomial features
degree = 2
poly_features = PolynomialFeatures(degree=degree)
X_poly_train = poly_features.fit_transform(X_train)

# Train the model
model = LinearRegression()
model.fit(X_poly_train, y_train)

# Print model features (coefficients and intercept)
print("Model coefficients:", model.coef_)
print("Model intercept:", model.intercept_)

# Predict and plot
X_poly = poly_features.transform(X)
y_pred = model.predict(X_poly)

plt.scatter(X, y, color='blue', label='Original Data')
plt.scatter(X, y_pred, color='red', label='Model Prediction')
plt.title('MQ-4 to Nano Mapping')
plt.xlabel('MQ-4')
plt.ylabel('Nano')
plt.legend()
plt.show()
