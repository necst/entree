# This source file comes from the Conifer open-source project 
# (https://github.com/thesps/conifer)

from sklearn.datasets import load_iris
from sklearn.ensemble import GradientBoostingClassifier
import datetime
import onnxmltools
import onnx
import numpy as np
import entree
import util
import pytest

iris = load_iris()
X, y = iris.data, iris.target

def train_model():
    clf = GradientBoostingClassifier(n_estimators=20, learning_rate=1.0,
                                    max_depth=3, random_state=0).fit(X, y)
    return clf

skl_model = train_model()

def sklearn_model():
    return skl_model, skl_model

def onnx_model():
    from skl2onnx.common.data_types import FloatTensorType
    initial_type = [('float_input', FloatTensorType([None, X.shape[1]]))]
    clf = onnxmltools.convert_sklearn(skl_model, 'iris model', initial_types=initial_type)
    onnx.save(clf, 'iris_bdt.onnx')
    return clf, 'iris_bdt.onnx'

models = {'sklearn' : sklearn_model,
          'onnx'    : onnx_model}

predicts = {'sklearn' : util.predict,
            'onnx'    : util.predict_onnx}

frontends = {'sklearn' : entree.converters.sklearn,
            'onnx'    : entree.converters.onnx}

backends = {'xilinxhls' : entree.backends.xilinxhls,
            'vhdl'      : entree.backends.vhdl}

@pytest.mark.parametrize('frontend', ['sklearn', 'onnx'])
@pytest.mark.parametrize('backend', ['xilinxhls', 'vhdl'])
def test_multiclass(frontend, backend):
    clf, predictor = models[frontend]()

    # Create a entree config
    cfg = entree.backends.xilinxhls.auto_config()
    # Set the output directory to something unique
    cfg['OutputDir'] = 'prj_{}'.format(int(datetime.datetime.now().timestamp()))
    cfg['Precision'] = 'ap_fixed<32,16,AP_RND,AP_SAT>'
    cfg['XilinxPart'] = 'xcu250-figd2104-2L-e'
    cnf = entree.model(clf, frontends[frontend],
                        backends[backend], cfg)
    cnf.compile()

    y_hls, y_skl = predicts[frontend](predictor, X, y, cnf)
    np.testing.assert_allclose(y_hls, y_skl, rtol=1e-2, atol=1e-2)
    cnf.build()