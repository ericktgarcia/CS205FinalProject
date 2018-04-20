import sys
import numpy as np
import torch 
import torch.nn as nn
import torchvision.datasets as dsets
import torchvision.transforms as transforms
from torch.autograd import Variable


## Hyper Parameters
num_epochs = 5
channels = 2
learning_rate = 0.05
window_size = int(sys.argv[1])
kernel_size = int(sys.argv[2])

def gen_Xy(dim,scale=100,temp_dep=10):
    X = np.random.uniform(-scale,scale,size=(dim[0],dim[1]))
    y = []
    for idx in range(X.shape[0]):
        y.append(np.mean(X[idx:idx+temp_dep, :]))

    y = np.array(y)
    return X, y

def window(X,y,window_size=10):
    n_windows = X.shape[0]-window_size
    X_windowed = np.array([X[i:i+window_size] for i in range(n_windows)])
    return X_windowed, y[:-window_size]


# CNN Model (2 conv layer)
class CNN(nn.Module):
    def __init__(self):
        super(CNN, self).__init__()
        self.layer1 = nn.Sequential(
            nn.Conv1d(channels, 16, kernel_size=kernel_size),
            nn.BatchNorm1d(16),
            nn.ReLU(),
            nn.MaxPool1d(2))
        self.layer2 = nn.Sequential(
            nn.Conv1d(16, 32, kernel_size=kernel_size, padding=2),
            nn.BatchNorm1d(32),
            nn.ReLU(),
#             nn.MaxPool1d(2))
            nn.AdaptiveAvgPool1d(4))
        self.fc = nn.Linear(128, 1)
        
    def forward(self, x):
        out = self.layer1(x)
        out = self.layer2(out)
        out = out.view(out.size(0), -1)
        out = self.fc(out)
        return out

        
cnn = CNN()

# Loss and Optimizer
criterion = nn.MSELoss()
optimizer = torch.optim.Adam(cnn.parameters(), lr=learning_rate)

# Train the Model
for epoch in range(num_epochs):
    X,y = gen_Xy([2000,channels],temp_dep=20)
    X,y = window(X,y,window_size=window_size)
    X = np.swapaxes(X,1,2)
    traces = Variable(torch.from_numpy(X).float())
    result = Variable(torch.from_numpy(y).float())
#     for i, (images, labels) in enumerate(train_loader):
#         images = Variable(images)
#         labels = Variable(labels)
        
        # Forward + Backward + Optimize
    optimizer.zero_grad()
    outputs = cnn(traces)
    loss = criterion(outputs, result)
    loss.backward()
    optimizer.step()
        
#print ('Epoch [%d/%d], Loss: %.4f' %(epoch+1, num_epochs, loss.data[0]))
print(loss.data[0])
