import sys
import numpy as np

sys.argv.pop(0)
for arg in sys.argv:
    print(int(float(arg)))

print(np.random.random())
