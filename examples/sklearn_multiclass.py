# This source file comes from the Conifer open-source project 
# (https://github.com/thesps/conifer)

from sklearn.datasets import load_iris
from sklearn.ensemble import GradientBoostingClassifier
import entree
import datetime

iris = load_iris()
X, y = iris.data, iris.target

clf = GradientBoostingClassifier(n_estimators=20, learning_rate=1.0,
                                 max_depth=3, random_state=0).fit(X, y)

# Create a entree config
cfg = entree.backends.xilinxhls.auto_config()
# Set the output directory to something unique
cfg['OutputDir'] = 'prj_{}'.format(int(datetime.datetime.now().timestamp()))

model = entree.model(clf, entree.converters.sklearn,
                      entree.backends.xilinxhls, cfg)
model.compile()

# Run HLS C Simulation and get the output
y_hls = model.decision_function(X)
y_skl = clf.decision_function(X)

# Synthesize the model
model.build()
