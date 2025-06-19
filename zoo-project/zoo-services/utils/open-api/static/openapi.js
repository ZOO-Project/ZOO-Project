var socket;
function addElementToList(){
    var lClosure=arguments[0];
    var isOver=false;
    var cnt=0;
    lClosure.parent().parent().find("div").each(function(){
        if(isOver) return;
        if($(this).hasClass("btn-group")) isOver=true;
        if($(this).hasClass("input-group")){
            lClosure.parent().parent().append($(this)[0].outerHTML);
            cnt++;
        }
    });
    if(lClosure.parent().parent().find(".input-group").length>cnt)
        lClosure.next().attr("disabled",false);
    else
        lClosure.next().attr("disabled",true);
}
function delElementToList(){
    var lClosure=arguments[0];
    var isOver=false;
    var cnt=0;
    lClosure.parent().parent().find("div").each(function(){
        if(isOver) return;
        if($(this).hasClass("btn-group")) isOver=true;
        if($(this).hasClass("input-group")) cnt++;
    });
    if(lClosure.parent().parent().find(".input-group").length>cnt)
        for(var i=0;i<cnt;i++)
            lClosure.parent().parent().find(".input-group").last().remove();
    if(lClosure.parent().parent().find(".input-group").length==cnt)
        lClosure.attr("disabled",true);
    else
        lClosure.attr("disabled",false);
}

function loadRequest(){
    var executionMode=$("select[name='main_value_mode']").val();
    var requestObject={
    //"id": System["JSON_STR"]["id"],
        "inputs":{},
        "outputs":{},
        "subscriber":{}/*,
        "mode": $("select[name='main_value_mode']").val(),
        "response": $("select[name='main_value_format']").val()*/
    };
    if($('input[name="oapi_ioAsArray"]').val()=="true"){
        requestObject["inputs"]=[];
        requestObject["outputs"]=[];
    }
    for(var i in System["JSON_STR"]["inputs"]){
    //for(var i=0;i < System["JSON_STR"]["inputs"].length;i++){
        var cName=i.replace(".","_");
        var selector="input[name='input_value_"+cName+"'],"+
            "select[name='input_value_"+cName+"']";
        if($(selector).val()!=""){
            $(selector).each(function(){
                var cInput={};
                if($('input[name="oapi_ioAsArray"]').val()=="true")
                    cInput={"id": System["JSON_STR"]["inputs"][i]["id"], "input": {}};
                if(System["JSON_STR"]["inputs"][i]["schema"]["oneOf"]){
                    var selector1="input[name='input_format_"+cName+"'],"+
                    "select[name='input_format_"+cName+"']";
                    console.log($(this).parent().prev().find("select").val());
                    if($('input[name="oapi_ioAsArray"]').val()=="true"){
                        cInput["input"]["format"]={
                            "mediaType": $(this).parent().prev().find("select").val()
                        };
                        cInput["input"]["href"]=$(this).val();
                    }else{
                        cInput["type"]=$(this).parent().prev().find("select").val();
                        cInput["href"]=$(this).val();
                    }
                }
                else{
                    if("type" in System["JSON_STR"]["inputs"][i]["schema"] && System["JSON_STR"]["inputs"][i]["schema"]["type"]!="object"){
                        console.log(System["JSON_STR"]["inputs"][i]["schema"]["type"]);
                        cInput=$(this).val();
                    }else{
                        var keys=[
                                "_minx]",
                                "_miny]",
                                "_maxx]",
                                "_maxy]"
                        ];
                        for(var iCnt=0;iCnt<4;iCnt++){
                            if($("input[name="+$(this).attr("name")+keys[iCnt]).val()=="")
                                    return;
                        }
                        cInput["crs"]=$(this).val();
                        cInput["bbox"]=[
                            parseFloat($("input[name="+$(this).attr("name")+"_minx]").val()),
                            parseFloat($("input[name="+$(this).attr("name")+"_miny]").val()),
                            parseFloat($("input[name="+$(this).attr("name")+"_maxx]").val()),
                            parseFloat($("input[name="+$(this).attr("name")+"_maxy]").val())
                        ];
                    }
                }
                console.log(cInput);
                if($('input[name="oapi_ioAsArray"]').val()=="true")
                    requestObject["inputs"].push(cInput);
                else{
                    if(!requestObject["inputs"][System["JSON_STR"]["inputs"][i]["id"]])
                        requestObject["inputs"][System["JSON_STR"]["inputs"][i]["id"]]=cInput;
                    else{
                        if(!Array.isArray(requestObject["inputs"][System["JSON_STR"]["inputs"][i]["id"]])){
                            var saveObject=requestObject["inputs"][System["JSON_STR"]["inputs"][i]["id"]];
                            requestObject["inputs"][System["JSON_STR"]["inputs"][i]["id"]]=[saveObject];
                        }
                        requestObject["inputs"][System["JSON_STR"]["inputs"][i]["id"]].push(cInput);
                    }
                }
            });
        }
    }
    //console.log(System["JSON_STR"]["outputs"]);
    for(var i in System["JSON_STR"]["outputs"]){
        var cOutput={};
        console.log($('input[name="oapi_ioAsArray"]').val()=="true");
        if($('input[name="oapi_ioAsArray"]').val()=="true")
        cOutput={"id": i};
        var cName=i.replace(/\./g,"_");
        if(System["JSON_STR"]["outputs"][i]["schema"]["oneOf"]){
            var selector="select[name='format_"+cName+"']";
            cOutput["format"]={
                "mediaType": $(selector).val()
            };
        }
        else{
        /*if(System["JSON_STR"]["outputs"][i]["schema"]["type"]!="object"){
        cOutput["dataType"]={
            "name": System["JSON_STR"]["outputs"][i]["schema"]["literalDataDomains"][0]["dataType"]["name"]
        };
        }*/
        }
        var selector1="select[name='transmission_"+cName+"']";
        cOutput["transmissionMode"]=$(selector1).val();
        if($('input[name="oapi_ioAsArray"]').val()=="true")
            requestObject["outputs"].push(cOutput);
        else
            requestObject["outputs"][i]=cOutput;
    }
    
    if($("input[name='main_value_successUri']").val()!="")
        requestObject["subscriber"]["successUri"]=$("input[name='main_value_successUri']").val();
    if($("input[name='main_value_inProgressUri']").val()!="")
        requestObject["subscriber"]["inProgressUri"]=$("input[name='main_value_inProgressUri']").val();
    if($("input[name='main_value_failedUri']").val()!="")
        requestObject["subscriber"]["failedUri"]=$("input[name='main_value_failedUri']").val();
    $(".modal").find("textarea").first().val(js_beautify(JSON.stringify(requestObject)));
    $("#requestModal").modal('toggle');
    $('#result').html("");
    $("#requestModal").find(".btn-primary").off('click');
    var firstRun=true;
    $("#requestModal").find(".btn-primary").click(function(){
        $('#result').html("");
        if(!socket && executionMode!="sync")
            socket = new WebSocket($('input[name="oapi_wsUrl"]').val());
        else
            if(executionMode=="sync"){
                var localHeaders={"Prefer": "return="+($("select[name='main_value_format']").val()=="raw"?"minimal":"representation")};
                $.ajax({
                    headers: localHeaders,
                    contentType: "application/json",
                    data: $("textarea").val(),
                    type: "POST",
                    url: $('input[name="oapi_jobUrl"]').val(),
                    success: function (msg) {
                        console.log(msg);
                        var cObj=msg;
                        $('#result').html(js_beautify(JSON.stringify(msg)));
                                },
                                error: function(){
                        console.log(arguments);
                        $('#result').html(js_beautify(JSON.stringify(arguments[0].responseJSON)));
                    },
                });
            }
        if(executionMode=="sync"){
            return;
        }
        console.log(firstRun);
        socket.onopen = function () {
            console.log('Connected!');
            socket.send("SUB "+$('input[name="oapi_reqID"]').val());
        };
        socket.onmessage = function(event) {
            //console.log('MESSAGE: ' + event.data);
            if(event.data=="1"){
                var localHeaders={};
                if(executionMode!="sync"){
                    localHeaders={"Prefer": "respond-async;return="+($("select[name='main_value_format']").val()=="raw"?"minimal":"representation")};
                }else{
                    localHeaders={"Prefer": "return="+($("select[name='main_value_format']").val()=="raw"?"minimal":"representation")};
                }
                console.log(localHeaders);
                
                $.ajax({
                    contentType: "application/json",
                    headers: localHeaders,
                    data: $("textarea").val(),
                    type: "POST",
                    url: $('input[name="oapi_jobUrl"]').val(),
                    success: function (msg) {
                        console.log(msg);
                    },
                    error: function(){
                        console.log(arguments);
                    },
                });
            }
            else{
                //progressBar
                $("#progress_details").show();
                var cObj=JSON.parse(event.data);
                if(cObj["jobID"]){
                    //console.log('MESSAGE: ' + event.data);
                    $("#prgress_description").html(cObj["jobID"]+": "+cObj["message"]);
                    $(".progress-bar").attr("aria-valuenow",cObj["progress"]);
                    $(".progress-bar").css("width",cObj["progress"]+"%");
                }else{
                    //console.log('MESSAGE (to close): ' + event.data);
                    $("#progress_details").hide();
                    if(cObj)
                        $('#result').html(js_beautify(JSON.stringify(cObj)));
                    else
                        $('#result').html(cObj["message"]);
                    socket.close();
                    socket=false;
                }
            }
        };
    });
}
