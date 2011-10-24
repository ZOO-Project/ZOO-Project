
function hellojs(conf,inputs,outputs){
	outputs["result"]["value"]="Hello "+inputs["S"]["value"]+" from the JS World !";
	//SERVICE_SUCEEDED
	return Array(3,outputs);
}

function hellojs1(conf,inputs,outputs){
	outputs["result"]["value"]="Hello "+inputs["S"]["value"]+" from the JS World !";
	//SERVICE_SUCEEDED
	return {"result":3,"outputs": outputs};
}

