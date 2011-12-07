import sys
def HelloPy(conf,inputs,outputs):
	outputs["Result"]["value"]="Hello "+inputs["a"]["value"]+" from Python World !"
	return 3

