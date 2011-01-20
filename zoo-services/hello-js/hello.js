
function hellojs(conf,inputs,outputs){
	outputs[0]["result"]["value"]="Hello "+inputs[0]["S"]["value"]+" from the JS World !";
	//SERVICE_SUCEEDED
	return Array(3,outputs);
}

function hellojs1(conf,inputs,outputs){
	outputs[0]["result"]["value"]="Hello "+inputs[0]["S"]["value"]+" from the JS World !";
	//SERVICE_SUCEEDED
	return {"result":3,"outputs": outputs};
}

