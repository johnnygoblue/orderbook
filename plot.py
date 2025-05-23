import pandas as pd
import matplotlib.pyplot as plt

df = pd.read_csv('orderbook_benchmark.csv')
fig, axes = plt.subplots(2, 2, figsize=(12, 10))

# Add Orders
df.plot.bar(x='Implementation', y='Add Mean', yerr='Add StdDev', capsize=4, ax=axes[0,0], title='Add Orders (μs)')
# Modify Orders
df.plot.bar(x='Implementation', y='Modify Mean', yerr='Modify StdDev', capsize=4, ax=axes[0,1], title='Modify Orders (μs)')
# Delete Orders
df.plot.bar(x='Implementation', y='Delete Mean', yerr='Delete StdDev', capsize=4, ax=axes[1,0], title='Delete Orders (μs)')
# Best Price
df.plot.bar(x='Implementation', y='BestPrice Mean', yerr='BestPrice StdDev', capsize=4, ax=axes[1,1], title='Best Price Lookup (ns)')

plt.tight_layout()
plt.savefig('orderbook_benchmark.png')
plt.show()
