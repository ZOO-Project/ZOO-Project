// Filename: zoo-process.js
/**
 * Author : Samuel Souk aloun
 *
 * Copyright (c) 2014 GeoLabs SARL
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

define([
    'xml2json', 'queryString', 'wpsPayload', 'utils'
], function(X2JS, qs, wpsPayload, utils) {

    var ZooProcess = function(params) {
        
        /**
         * Private
         */        
        
        var _x2js = new X2JS({
            arrayAccessFormPaths: [
            'ProcessDescriptions.ProcessDescription.DataInputs.Input',
            'ProcessDescriptions.ProcessDescription.DataInputs.Input.ComplexData.Supported.Format',
            'ProcessDescriptions.ProcessDescription.ProcessOutputs.Output',
            'ProcessDescriptions.ProcessDescription.ProcessOutputs.Output.ComplexOutput.Supported.Format',
            'Capabilities.ServiceIdentification.Keywords'
            ],   
        });

        
        /**
         * Public
         */
         
        this.url = params.url;
        
        this.statusLocation = {};
        this.launched = {};
        this.terminated = {};
        this.percent = {};
        this.delay = params.delay || 2000,
        
        //
        this.describeProcess = function(params) {
            var closure = this;

            if (!params.hasOwnProperty('type')) {
                params.type = 'GET';
            }

            var zoo_request_params = {
                Identifier: params.identifier,
                metapath: params.metapath ? params.metapath : '',
                request: 'DescribeProcess',
                service: 'WPS',
                version: '1.0.0',
            }

            this.request(zoo_request_params, params.success, params.error, params.type);
        };
        
        //
        this.getCapabilities = function(params) {
            var closure = this;

            if (!params.hasOwnProperty('type')) {
                params.type = 'GET';
            }

            // http://zoo-server/cgi-bin/zoo_loader.cgi?ServiceProvider=&metapath=&Service=WPS&Request=GetCapabilities&Version=1.0.0
            var zoo_request_params = {
                ServiceProvider: '',
                metapath: params.metapath ? params.metapath : '',
                request: 'GetCapabilities',
                service: 'WPS',
                version: '1.0.0',
            }

            this.request(zoo_request_params, params.success, params.error, params.type);
        };
        
        //
        this.execute = function(params) {
            var closure = this;
            console.log("======== Execute "+params.identifier);
            console.log(params);

            if (!params.hasOwnProperty('type')) {
                params.type = 'GET';
            }

            var zoo_request_params = {
                request: 'Execute',
                service: 'WPS',
                version: '1.0.0',
                Identifier: params.identifier,
                DataInputs: params.dataInputs ? params.dataInputs : '',
                DataOutputs: params.dataOutputs ? params.dataOutputs : '',

                //storeExecuteResponse: params.storeExecuteResponse ? 'true' : 'false',
                //status: params.status ? 'true' : 'false',
            }


            if (params.hasOwnProperty('responseDocument')) {
                zoo_request_params.ResponseDocument = params.responseDocument;
            }
            if (params.hasOwnProperty('storeExecuteResponse') &&  params.storeExecuteResponse) {
                zoo_request_params.storeExecuteResponse = 'true';
            }
            if (params.hasOwnProperty('status') &&  params.status) {
                zoo_request_params.status = 'true';
            }
            if (params.hasOwnProperty('lineage') &&  params.lineage) {
                zoo_request_params.lineage = 'true';
            }


            this.request(zoo_request_params, params.success, params.error, params.type);
        };

        
        //
        this.request = function(params, onSuccess, onError, type) {
            var closure = this;
            console.log('======== REQUEST type='+type);
            console.log(params);

            var url = this.url;
            var payload;
            var headers;

            if (type == 'GET') {
                url += '?' + this.getQueryString(params);
            } else if (type == 'POST') {
                payload = wpsPayload.getPayload(params);
                console.log("======== POST PAYLOAD ========");
                console.log(payload);

                headers = {
                    "Content-Type": "text/xml"        
                };
            }
            
            console.log("ajax url: "+url);

            $.ajax({
                type: type,
                url: url,
                dataType: "xml",
                data: payload,
                headers: headers
            })
            .always(
                function() {
                    //console.log("ALWAYS");
                }
            )
            .fail(
                function(jqXHR, textStatus, errorThrown) {
                    console.log("======== ERROR ========"); 
                    onError(jqXHR);       
                }
            )
            .done(
                function(data, textStatus, jqXHR) {
                    console.log("======== SUCCESS ========");
                    //console.log(data);
                    console.log(utils.xmlToString(data));

                    // TODO: move this transformation
                    data = _x2js.xml2json( data );

                    console.log(data);
                    //------

                    var launched;

                    if (params.storeExecuteResponse == 'true' && params.status == 'true') {
                        launched = closure.parseStatusLocation(data);            
                        console.log(launched);
                        closure.statusLocation[launched.sid] = launched.statusLocation;

                        if (launched.hasOwnProperty('sid') && !closure.launched.hasOwnProperty(launched.sid)) {                    
                            closure.launched[launched.sid] = launched.params;
                            //closure.emit('launched', launched);
                        }           
                    }
                    onSuccess(data, launched);
            });
        };
        
        //
        this.watch = function(sid, handlers) {
            //onPercentCompleted, onProcessSucceeded, onError
            var closure = this;

            console.log("WATCH: "+sid);

            function onSuccess(data) {
                console.log("++++ getStatus SUCCESS "+sid);
                console.log(data);

                if (data.ExecuteResponse.Status.ProcessStarted) {
                    console.log("#### ProcessStarted");

                    var ret = {
                        sid: sid,
                        percentCompleted: data.ExecuteResponse.Status.ProcessStarted._percentCompleted,
                        text: data.ExecuteResponse.Status.ProcessStarted.__text,
                        creationTime: data.ExecuteResponse.Status.ProcessStarted._creationTime,
                    };

                    closure.percent[sid] = ret.percentCompleted;
                    //closure.emit('percent', ret);

                    if (handlers.onPercentCompleted instanceof Function) {
                        handlers.onPercentCompleted(ret);
                    }
                }
                else if (data.ExecuteResponse.Status.ProcessSucceeded) {
                console.log("#### ProcessSucceeded");

                    var text = data.ExecuteResponse.Status.ProcessSucceeded.__text;
                    closure.terminated[sid] = true;

                    ret = {
                        sid: sid,
                        text: text,
                        result: data
                    };

                    //closure.emit('success', ret);
                    if (handlers.onProcessSucceeded instanceof Function) {
                        handlers.onProcessSucceeded(ret);
                    }
                }
                else {
                    console.log("#### UNHANDLED EXCEPTION");
                    closure.terminated[sid] = true;
                    ret = {
                        sid: sid,
                        code: 'BAD',
                        text: 'UNHANDLED EXCEPTION'
                    };

                    //closure.emit('exception', ret);
                    if (handlers.onError instanceof Function) {
                        handlers.onError(ret);
                    }
                }    
            }

            function onError(data) {
                console.log("++++ getStatus ERROR "+sid);
                console.log(data);
            }

            function ping(sid) {
                console.log("PING: "+sid);
                closure.getStatus(sid, onSuccess, onError);
                if (closure.terminated[sid]) {
                    console.log("++++ getStatus TERMINATED "+sid);            
                }
                else if (!closure.percent.hasOwnProperty(sid) || closure.percent[sid]<100) {
                    setTimeout( function() {
                        ping(sid);
                    }, closure.delay);            
                } else {
                    console.log(closure.percent);
                }
            }

            ping(sid);
        };
        
        //
        this.getStatus = function(sid, onSuccess, onError) {
            var closure = this;

            console.log("GET STATUS: "+sid);

            if (closure.terminated[sid]) {
                console.log("DEBUG TERMINATED");
                return;
            }
            if (!closure.launched[sid]) {
                console.log("DEBUG LAUNCHED");
                return;
            }
            console.log("PARSE URI: "+closure.statusLocation[sid]);
            var parsed_url = utils.parseUri(closure.statusLocation[sid]);
            console.log(parsed_url);
            zoo_request_params = parsed_url.queryKey;

            this.request(zoo_request_params, onSuccess, onError, 'GET');
        };
        
        //
        this.getQueryString = function(params) {

            var ret = '';

            // TODO: serialize attributes
            serializeInputs = function(obj) {
              console.log("SERIALIZE dataInputs");
              console.log(obj);
              if($.type(obj) === "string") {
                return obj;
              }
              var str = [];
              for(var p in obj){
                if (obj.hasOwnProperty(p)) {
                  //str.push(encodeURIComponent(p) + "=" + encodeURIComponent(obj[p]));
                  str.push(p + "=" + obj[p]);
                }
              }
              return str.join(";");
            }

            var responseDocument = params.ResponseDocument;
            var tmp_params = {};

            var objectKeys = Object.keys || function (obj) {
                var keys = [];
                for (var key in obj) keys.push(key);
                return keys;
            };

            var skip = {
                'DataInputs': true,
                'DataOuptputs': true,
                'ResponseDocument': true,
            }
            var keys = objectKeys(params);
            for (var i = 0; i < keys.length; i++) {
                var key = keys[i];
                if (skip.hasOwnProperty(key) && skip[key]) {
                    continue;
                }
                if (params.hasOwnProperty(key)) {
                    tmp_params[key] = params[key];
                }
            }

            ret = qs.stringify(tmp_params);

            //req_options.path = req_options.path.replace("&DataInputs=sid%3D", "&DataInputs=sid=")
            if (params.hasOwnProperty('DataInputs')) {
              //var dataInputs = params.DataInputs;
              var dataInputs = serializeInputs(params.DataInputs);
              console.log("dataInputs: "+dataInputs);
              ret += '&DataInputs=' + dataInputs;
            }
            /*
            if (params.hasOwnProperty('DataOutputs')) {
              var dataOutputs = serializeOutputs(params.DataOutputs);
              console.log("dataOutputs: "+dataOutputs);
              ret += '&DataOutputs=' + dataOutputs;
            }
            */
            //ResponseDocument=Result ou RawDataOutput=Result

            if (params.ResponseDocument) {
                ret += '&ResponseDocument=' + params.ResponseDocument;
            }

            return ret;
        };
        
        this.parseStatusLocation = function(data) {
            var closure = this;

            if (statusLocation = data.ExecuteResponse._statusLocation) {
                console.log("statusLocation: "+statusLocation);

                var parsed_url = utils.parseUri(statusLocation);
                console.log(parsed_url);
                var parsed_params = parsed_url.queryKey;

                return {sid: parsed_url.queryKey.DataInputs, params: parsed_params, statusLocation: statusLocation};
            }
        };        
    };
    

    return ZooProcess;

});