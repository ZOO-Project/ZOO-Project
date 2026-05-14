#
# Author : Gérald Fenoy
#
# Copyright 2025 GeoLabs. All rights reserved.
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the
# "Software"), to deal in the Software without restriction, including with
# out limitation the rights to use, copy, modify, merge, publish,
# distribute, sublicense, and/or sell copies of the Software, and to
# permit persons to whom the Software is furnished to do so, subject to
# the following conditions:
#
# The above copyright notice and this permission notice shall be included
# in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
# OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
# IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
# CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
# TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
# SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#

from flask import Flask, request, jsonify
import subprocess
from urllib.parse import unquote
import json

app = Flask(__name__)

@app.route('/<path:image_name>', methods=['GET'])
def get_image_config(image_name):
    try:
        decoded_image_name = unquote(image_name)
        
        # Execute the command "crane config <image_name>"
        result = subprocess.run(['crane', 'config', decoded_image_name], capture_output=True, text=True)
        
        if result.returncode == 0:
            result = json.loads(result.stdout)
            return jsonify({"status": "success", "config": result["config"]})
        else:
            return jsonify({"status": "error", "message": result.stderr}), 400
    except Exception as e:
        return jsonify({"status": "error", "message": str(e)}), 500

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000)
