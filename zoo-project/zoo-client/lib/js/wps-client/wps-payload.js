// Filename: wps-payload.js
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
    'jquery', 'utils',
    'hgn!tpl/payload_GetCapabilities',
    'hgn!tpl/payload_DescribeProcess',
    'hgn!tpl/payload_Execute',
    
    
], function($, utils, tplGetCapabilities, tplDescribeProcess, tplExecute) {
    
    //
    return {
        
        //
        getPayload: function(params) {
            if (params.request == 'DescribeProcess') {
                return this.getPayload_DescribeProcess(params);
            } else if (params.request == 'GetCapabilities') {
                return this.getPayload_GetCapabilities(params);
            } else if (params.request == 'Execute') {
                return this.getPayload_Execute(params);
            } else {
                console.log("#### UNKNOWN REQUEST ####");
            }
        },
        
        //
        getPayload_DescribeProcess: function(params) {
            if (params.Identifier) {
                if ($.isArray(params.Identifier)) {
                    return tplDescribeProcess({identifiers: params.Identifier});
                }
                else {
                    return tplDescribeProcess({identifiers: [params.Identifier]});
                }
            }
            // TODO: no Identifier
        },

        //
        getPayload_GetCapabilities: function(params) {
            return tplGetCapabilities();
        },

        //
        getPayload_Execute: function(params) {
            //console.log(params);
            //console.log("==== INPUTS ====");
            if (params.DataInputs) {
                //console.log(params.DataInputs);

                for (var i = 0; i < params.DataInputs.length; i++) {
                    
                    /*
                	 * Set some default values and flags.
                	 */
                	if (params.DataInputs[i].type == 'bbox') {
                	    if (!params.DataInputs[i].crs) {
                    		params.DataInputs[i].crs = "EPSG:4326";
                    	}

                    	if (!params.DataInputs[i].dimension) {
                    		params.DataInputs[i].dimension = 2;
                    	}
            	    }

                    if (params.DataInputs[i].type) {
                        params.DataInputs[i]['is_'+params.DataInputs[i].type] = true;
                    }
                    
                    // Complex data from payload callback.
                    if (params.DataInputs[i].complexPayload_callback) {
                        params.DataInputs[i].complexPayload = window[params.DataInputs[i].complexPayload_callback];
                    }
                    
                    // Complex data from reference.
                    if (params.DataInputs[i].href) {
                        params.DataInputs[i].is_reference = true;
                        //params.DataInputs[i].href = utils.encodeXML(params.DataInputs[i].href);
                        if (params.DataInputs[i].method == 'POST') {
                            params.DataInputs[i].is_post = true;
                        } else {
                            params.DataInputs[i].is_get = true;
                        }
                    }
                    else {                        
                        // Complex data, embeded
                    }
                } // for i loop
            }

            //console.log("==== OUTPUTS ====");
            if (params.DataOutputs || params.storeExecuteResponse || params.status || params.lineage) {
                console.log(params.DataOutputs);
                
                for (var i = 0; i < params.DataOutputs.length; i++) {
                    //console.log(params.DataOutputs[i]);
                    
                    if (params.DataOutputs[i].type) {
                        params.DataOutputs[i]['is_'+params.DataOutputs[i].type] = true;
                    }
                }
            }
            
            return tplExecute(params);
        },
        

    };

});



