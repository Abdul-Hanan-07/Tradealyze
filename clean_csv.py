import pandas as pd
import re

# 1. Read the raw file as plain text to fix the bad decimals
with open('RecordBook.csv', 'r') as file:
    content = file.read()

# 2. Use a regular expression to fix the double-decimal bug (e.g., 0.7.445 -> 7.445)
# This finds ",0.X.Y" and safely replaces it with ",X.Y"
cleaned_content = re.sub(r',0\.([0-9]+)\.([0-9]+)', r',\1.\2', content)

# 3. Save it to a new file so you don't lose the original
with open('RecordBook_Clean.csv', 'w') as file:
    file.write(cleaned_content)

print("✅ Success! Your data has been cleaned and saved as 'RecordBook_Clean.csv'.")
print("Update your C++ OrderBook to use this new file!")