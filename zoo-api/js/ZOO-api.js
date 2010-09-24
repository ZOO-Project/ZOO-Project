/**
 * Author : René-Luc D'Hont
 *
 * Copyright 2010 3liz SARL. All rights reserved.
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

ZOO = {
  SERVICE_ACCEPTED: 0,
  SERVICE_STARTED: 1,
  SERVICE_PAUSED: 2,
  SERVICE_SUCCEEDED: 3,
  SERVICE_FAILED: 4,
  removeItem: function(array, item) {
    for(var i = array.length - 1; i >= 0; i--) {
        if(array[i] == item) {
            array.splice(i,1);
        }
    }
    return array;
  },
  indexOf: function(array, obj) {
    for(var i=0, len=array.length; i<len; i++) {
      if (array[i] == obj)
        return i;
    }
    return -1;   
  },
  extend: function(destination, source) {
    destination = destination || {};
    if(source) {
      for(var property in source) {
        var value = source[property];
        if(value !== undefined)
          destination[property] = value;
      }
    }
    return destination;
  },
  Class: function() {
    var Class = function() {
      this.initialize.apply(this, arguments);
    };
    var extended = {};
    var parent;
    for(var i=0; i<arguments.length; ++i) {
      if(typeof arguments[i] == "function") {
        // get the prototype of the superclass
              parent = arguments[i].prototype;
      } else {
        // in this case we're extending with the prototype
        parent = arguments[i];
      }
      ZOO.extend(extended, parent);
    }
    Class.prototype = extended;

    return Class;
  },
  UpdateStatus: function(env,value) {
    return ZOOUpdateStatus(env,value);
  }
};

};

ZOO.String = {
  startsWith: function(str, sub) {
    return (str.indexOf(sub) == 0);
  },
  contains: function(str, sub) {
    return (str.indexOf(sub) != -1);
  },
  trim: function(str) {
    return str.replace(/^\s\s*/, '').replace(/\s\s*$/, '');
  },
  camelize: function(str) {
    var oStringList = str.split('-');
    var camelizedString = oStringList[0];
    for (var i=1, len=oStringList.length; i<len; i++) {
      var s = oStringList[i];
      camelizedString += s.charAt(0).toUpperCase() + s.substring(1);
    }
    return camelizedString;
  },
  tokenRegEx:  /\$\{([\w.]+?)\}/g,
  numberRegEx: /^([+-]?)(?=\d|\.\d)\d*(\.\d*)?([Ee]([+-]?\d+))?$/,
  isNumeric: function(value) {
    return ZOO.String.numberRegEx.test(value);
  },
  numericIf: function(value) {
    return ZOO.String.isNumeric(value) ? parseFloat(value) : value;
  }
};

ZOO.Request = {
  Get: function(url,params) {
    var paramsArray = [];
    for (var key in params) {
      var value = params[key];
      if ((value != null) && (typeof value != 'function')) {
        var encodedValue;
        if (typeof value == 'object' && value.constructor == Array) {
          /* value is an array; encode items and separate with "," */
          var encodedItemArray = [];
          for (var itemIndex=0, len=value.length; itemIndex<len; itemIndex++) {
            encodedItemArray.push(encodeURIComponent(value[itemIndex]));
          }
          encodedValue = encodedItemArray.join(",");
        }
        else {
          /* value is a string; simply encode */
          encodedValue = encodeURIComponent(value);
        }
        paramsArray.push(encodeURIComponent(key) + "=" + encodedValue);
      }
    }
    var paramString = paramsArray.join("&");
    if(paramString.length > 0) {
      var separator = (url.indexOf('?') > -1) ? '&' : '?';
      url += separator + paramString;
    }
    return ZOORequest('GET',url);
  },
  Post: function(url,body,headers) {
    if(!(headers instanceof Array)) {
      var headersArray = [];
      for (var hname in headers) {
        headersArray.push(name+': '+headers[name]); 
      }
      headers = headersArray;
    }
    return ZOORequest('POST',url,body,headers);
  }
};

ZOO.Bounds = ZOO.Class({
  left: null,
  bottom: null,
  right: null,
  top: null,
  initialize: function(left, bottom, right, top) {
    if (left != null)
      this.left = parseFloat(left);
    if (bottom != null)
      this.bottom = parseFloat(bottom);
    if (right != null)
      this.right = parseFloat(right);
    if (top != null)
      this.top = parseFloat(top);
  },
  clone:function() {
    return new ZOO.Bounds(this.left, this.bottom, 
                          this.right, this.top);
  },
  equals:function(bounds) {
    var equals = false;
    if (bounds != null)
        equals = ((this.left == bounds.left) && 
                  (this.right == bounds.right) &&
                  (this.top == bounds.top) && 
                  (this.bottom == bounds.bottom));
    return equals;
  },
  toString:function() {
    return ( "left-bottom=(" + this.left + "," + this.bottom + ")"
              + " right-top=(" + this.right + "," + this.top + ")" );
  },
  toArray: function() {
    return [this.left, this.bottom, this.right, this.top];
  },
  toBBOX:function(decimal) {
    if (decimal== null)
      decimal = 6; 
    var mult = Math.pow(10, decimal);
    var bbox = Math.round(this.left * mult) / mult + "," + 
               Math.round(this.bottom * mult) / mult + "," + 
               Math.round(this.right * mult) / mult + "," + 
               Math.round(this.top * mult) / mult;
    return bbox;
  },
  toGeometry: function() {
    return new ZOO.Geometry.Polygon([
      new ZOO.Geometry.LinearRing([
        new ZOO.Geometry.Point(this.left, this.bottom),
        new ZOO.Geometry.Point(this.right, this.bottom),
        new ZOO.Geometry.Point(this.right, this.top),
        new ZOO.Geometry.Point(this.left, this.top)
      ])
    ]);
  },
  getWidth:function() {
    return (this.right - this.left);
  },
  getHeight:function() {
    return (this.top - this.bottom);
  },
  add:function(x, y) {
    if ( (x == null) || (y == null) )
      return null;
    return new ZOO.Bounds(this.left + x, this.bottom + y,
                                 this.right + x, this.top + y);
  },
  extend:function(object) {
    var bounds = null;
    if (object) {
      // clear cached center location
      switch(object.CLASS_NAME) {
        case "ZOO.Geometry.Point":
          bounds = new ZOO.Bounds(object.x, object.y,
                                         object.x, object.y);
          break;
        case "ZOO.Bounds":    
          bounds = object;
          break;
      }
      if (bounds) {
        if ( (this.left == null) || (bounds.left < this.left))
          this.left = bounds.left;
        if ( (this.bottom == null) || (bounds.bottom < this.bottom) )
          this.bottom = bounds.bottom;
        if ( (this.right == null) || (bounds.right > this.right) )
          this.right = bounds.right;
        if ( (this.top == null) || (bounds.top > this.top) )
          this.top = bounds.top;
      }
    }
  },
  contains:function(x, y, inclusive) {
     //set default
     if (inclusive == null)
       inclusive = true;
     if (x == null || y == null)
       return false;
     x = parseFloat(x);
     y = parseFloat(y);

     var contains = false;
     if (inclusive)
       contains = ((x >= this.left) && (x <= this.right) && 
                   (y >= this.bottom) && (y <= this.top));
     else
       contains = ((x > this.left) && (x < this.right) && 
                   (y > this.bottom) && (y < this.top));
     return contains;
  },
  intersectsBounds:function(bounds, inclusive) {
    if (inclusive == null)
      inclusive = true;
    var intersects = false;
    var mightTouch = (
        this.left == bounds.right ||
        this.right == bounds.left ||
        this.top == bounds.bottom ||
        this.bottom == bounds.top
    );
    if (inclusive || !mightTouch) {
      var inBottom = (
          ((bounds.bottom >= this.bottom) && (bounds.bottom <= this.top)) ||
          ((this.bottom >= bounds.bottom) && (this.bottom <= bounds.top))
          );
      var inTop = (
          ((bounds.top >= this.bottom) && (bounds.top <= this.top)) ||
          ((this.top > bounds.bottom) && (this.top < bounds.top))
          );
      var inLeft = (
          ((bounds.left >= this.left) && (bounds.left <= this.right)) ||
          ((this.left >= bounds.left) && (this.left <= bounds.right))
          );
      var inRight = (
          ((bounds.right >= this.left) && (bounds.right <= this.right)) ||
          ((this.right >= bounds.left) && (this.right <= bounds.right))
          );
      intersects = ((inBottom || inTop) && (inLeft || inRight));
    }
    return intersects;
  },
  containsBounds:function(bounds, partial, inclusive) {
    if (partial == null)
      partial = false;
    if (inclusive == null)
      inclusive = true;
    var bottomLeft  = this.contains(bounds.left, bounds.bottom, inclusive);
    var bottomRight = this.contains(bounds.right, bounds.bottom, inclusive);
    var topLeft  = this.contains(bounds.left, bounds.top, inclusive);
    var topRight = this.contains(bounds.right, bounds.top, inclusive);
    return (partial) ? (bottomLeft || bottomRight || topLeft || topRight)
                     : (bottomLeft && bottomRight && topLeft && topRight);
  },
  CLASS_NAME: 'ZOO.Bounds'
});

ZOO.Projection = ZOO.Class({
  proj: null,
  projCode: null,
  initialize: function(projCode, options) {
    ZOO.extend(this, options);
    this.projCode = projCode;
    if (Proj4js) {
      this.proj = new Proj4js.Proj(projCode);
    }
  },
  getCode: function() {
    return this.proj ? this.proj.srsCode : this.projCode;
  },
  getUnits: function() {
    return this.proj ? this.proj.units : null;
  },
  toString: function() {
    return this.getCode();
  },
  equals: function(projection) {
    if (projection && projection.getCode)
      return this.getCode() == projection.getCode();
    else
      return false;
  },
  destroy: function() {
    this.proj = null;
    this.projCode = null;
  },
  CLASS_NAME: 'ZOO.Projection'
});
ZOO.Projection.transform = function(point, source, dest) {
    if (source.proj && dest.proj)
        point = Proj4js.transform(source.proj, dest.proj, point);
    return point;
};

ZOO.Format = ZOO.Class({
  options:null,
  externalProjection: null,
  internalProjection: null,
  data: null,
  keepData: false,
  initialize: function(options) {
    ZOO.extend(this, options);
    this.options = options;
  },
  destroy: function() {
  },
  read: function(data) {
  },
  write: function(data) {
  },
  CLASS_NAME: 'ZOO.Format'
});
ZOO.Format.WKT = ZOO.Class(ZOO.Format, {
  initialize: function(options) {
    this.regExes = {
      'typeStr': /^\s*(\w+)\s*\(\s*(.*)\s*\)\s*$/,
      'spaces': /\s+/,
      'parenComma': /\)\s*,\s*\(/,
      'doubleParenComma': /\)\s*\)\s*,\s*\(\s*\(/,  // can't use {2} here
      'trimParens': /^\s*\(?(.*?)\)?\s*$/
    };
    ZOO.Format.prototype.initialize.apply(this, [options]);
  },
  read: function(wkt) {
    var features, type, str;
    var matches = this.regExes.typeStr.exec(wkt);
    if(matches) {
      type = matches[1].toLowerCase();
      str = matches[2];
      if(this.parse[type]) {
        features = this.parse[type].apply(this, [str]);
      }
      if (this.internalProjection && this.externalProjection) {
        if (features && 
            features.CLASS_NAME == "ZOO.Feature") {
          features.geometry.transform(this.externalProjection,
                                      this.internalProjection);
        } else if (features &&
            type != "geometrycollection" &&
            typeof features == "object") {
          for (var i=0, len=features.length; i<len; i++) {
            var component = features[i];
            component.geometry.transform(this.externalProjection,
                                         this.internalProjection);
          }
        }
      }
    }    
    return features;
  },
  write: function(features) {
    var collection, geometry, type, data, isCollection;
    if(features.constructor == Array) {
      collection = features;
      isCollection = true;
    } else {
      collection = [features];
      isCollection = false;
    }
    var pieces = [];
    if(isCollection)
      pieces.push('GEOMETRYCOLLECTION(');
    for(var i=0, len=collection.length; i<len; ++i) {
      if(isCollection && i>0)
        pieces.push(',');
      geometry = collection[i].geometry;
      type = geometry.CLASS_NAME.split('.')[2].toLowerCase();
      if(!this.extract[type])
        return null;
      if (this.internalProjection && this.externalProjection) {
        geometry = geometry.clone();
        geometry.transform(this.internalProjection, 
                          this.externalProjection);
      }                       
      data = this.extract[type].apply(this, [geometry]);
      pieces.push(type.toUpperCase() + '(' + data + ')');
    }
    if(isCollection)
      pieces.push(')');
    return pieces.join('');
  },
  extract: {
    'point': function(point) {
      return point.x + ' ' + point.y;
    },
    'multipoint': function(multipoint) {
      var array = [];
      for(var i=0, len=multipoint.components.length; i<len; ++i) {
        array.push(this.extract.point.apply(this, [multipoint.components[i]]));
      }
      return array.join(',');
    },
    'linestring': function(linestring) {
      var array = [];
      for(var i=0, len=linestring.components.length; i<len; ++i) {
        array.push(this.extract.point.apply(this, [linestring.components[i]]));
      }
      return array.join(',');
    },
    'multilinestring': function(multilinestring) {
      var array = [];
      for(var i=0, len=multilinestring.components.length; i<len; ++i) {
        array.push('(' +
            this.extract.linestring.apply(this, [multilinestring.components[i]]) +
            ')');
      }
      return array.join(',');
    },
    'polygon': function(polygon) {
      var array = [];
      for(var i=0, len=polygon.components.length; i<len; ++i) {
        array.push('(' +
            this.extract.linestring.apply(this, [polygon.components[i]]) +
            ')');
      }
      return array.join(',');
    },
    'multipolygon': function(multipolygon) {
      var array = [];
      for(var i=0, len=multipolygon.components.length; i<len; ++i) {
        array.push('(' +
            this.extract.polygon.apply(this, [multipolygon.components[i]]) +
            ')');
      }
      return array.join(',');
    }
  },
  parse: {
    'point': function(str) {
       var coords = ZOO.String.trim(str).split(this.regExes.spaces);
            return new ZOO.Feature(
                new ZOO.Geometry.Point(coords[0], coords[1])
            );
    },
    'multipoint': function(str) {
       var points = ZOO.String.trim(str).split(',');
       var components = [];
       for(var i=0, len=points.length; i<len; ++i) {
         components.push(this.parse.point.apply(this, [points[i]]).geometry);
       }
       return new ZOO.Feature(
           new ZOO.Geometry.MultiPoint(components)
           );
    },
    'linestring': function(str) {
      var points = ZOO.String.trim(str).split(',');
      var components = [];
      for(var i=0, len=points.length; i<len; ++i) {
        components.push(this.parse.point.apply(this, [points[i]]).geometry);
      }
      return new ZOO.Feature(
          new ZOO.Geometry.LineString(components)
          );
    },
    'multilinestring': function(str) {
      var line;
      var lines = ZOO.String.trim(str).split(this.regExes.parenComma);
      var components = [];
      for(var i=0, len=lines.length; i<len; ++i) {
        line = lines[i].replace(this.regExes.trimParens, '$1');
        components.push(this.parse.linestring.apply(this, [line]).geometry);
      }
      return new ZOO.Feature(
          new ZOO.Geometry.MultiLineString(components)
          );
    },
    'polygon': function(str) {
       var ring, linestring, linearring;
       var rings = ZOO.String.trim(str).split(this.regExes.parenComma);
       var components = [];
       for(var i=0, len=rings.length; i<len; ++i) {
         ring = rings[i].replace(this.regExes.trimParens, '$1');
         linestring = this.parse.linestring.apply(this, [ring]).geometry;
         linearring = new ZOO.Geometry.LinearRing(linestring.components);
         components.push(linearring);
       }
       return new ZOO.Feature(
           new ZOO.Geometry.Polygon(components)
           );
    },
    'multipolygon': function(str) {
      var polygon;
      var polygons = ZOO.String.trim(str).split(this.regExes.doubleParenComma);
      var components = [];
      for(var i=0, len=polygons.length; i<len; ++i) {
        polygon = polygons[i].replace(this.regExes.trimParens, '$1');
        components.push(this.parse.polygon.apply(this, [polygon]).geometry);
      }
      return new ZOO.Feature(
          new ZOO.Geometry.MultiPolygon(components)
          );
    },
    'geometrycollection': function(str) {
      // separate components of the collection with |
      str = str.replace(/,\s*([A-Za-z])/g, '|$1');
      var wktArray = ZOO.String.trim(str).split('|');
      var components = [];
      for(var i=0, len=wktArray.length; i<len; ++i) {
        components.push(ZOO.Format.WKT.prototype.read.apply(this,[wktArray[i]]));
      }
      return components;
    }
  },
  CLASS_NAME: 'ZOO.Format.WKT'
});
ZOO.Format.JSON = ZOO.Class(ZOO.Format, {
  indent: "    ",
  space: " ",
  newline: "\n",
  level: 0,
  pretty: false,
  initialize: function(options) {
    ZOO.Format.prototype.initialize.apply(this, [options]);
  },
  read: function(json, filter) {
    try {
      if (/^[\],:{}\s]*$/.test(json.replace(/\\["\\\/bfnrtu]/g, '@').
                          replace(/"[^"\\\n\r]*"|true|false|null|-?\d+(?:\.\d*)?(?:[eE][+\-]?\d+)?/g, ']').
                          replace(/(?:^|:|,)(?:\s*\[)+/g, ''))) {
        var object = eval('(' + json + ')');
        if(typeof filter === 'function') {
          function walk(k, v) {
            if(v && typeof v === 'object') {
              for(var i in v) {
                if(v.hasOwnProperty(i)) {
                  v[i] = walk(i, v[i]);
                }
              }
            }
            return filter(k, v);
          }
          object = walk('', object);
        }
        if(this.keepData) {
          this.data = object;
        }
        return object;
      }
    } catch(e) {
      // Fall through if the regexp test fails.
    }
    return null;
  },
  write: function(value, pretty) {
    this.pretty = !!pretty;
    var json = null;
    var type = typeof value;
    if(this.serialize[type]) {
      try {
        json = this.serialize[type].apply(this, [value]);
      } catch(err) {
        //OpenLayers.Console.error("Trouble serializing: " + err);
      }
    }
    return json;
  },
  writeIndent: function() {
    var pieces = [];
    if(this.pretty) {
      for(var i=0; i<this.level; ++i) {
        pieces.push(this.indent);
      }
    }
    return pieces.join('');
  },
  writeNewline: function() {
    return (this.pretty) ? this.newline : '';
  },
  writeSpace: function() {
    return (this.pretty) ? this.space : '';
  },
  serialize: {
    'object': function(object) {
       // three special objects that we want to treat differently
       if(object == null)
         return "null";
       if(object.constructor == Date)
         return this.serialize.date.apply(this, [object]);
       if(object.constructor == Array)
         return this.serialize.array.apply(this, [object]);
       var pieces = ['{'];
       this.level += 1;
       var key, keyJSON, valueJSON;

       var addComma = false;
       for(key in object) {
         if(object.hasOwnProperty(key)) {
           // recursive calls need to allow for sub-classing
           keyJSON = ZOO.Format.JSON.prototype.write.apply(this,
                                                           [key, this.pretty]);
           valueJSON = ZOO.Format.JSON.prototype.write.apply(this,
                                                             [object[key], this.pretty]);
           if(keyJSON != null && valueJSON != null) {
             if(addComma)
               pieces.push(',');
             pieces.push(this.writeNewline(), this.writeIndent(),
                         keyJSON, ':', this.writeSpace(), valueJSON);
             addComma = true;
           }
         }
       }
       this.level -= 1;
       pieces.push(this.writeNewline(), this.writeIndent(), '}');
       return pieces.join('');
    },
    'array': function(array) {
      var json;
      var pieces = ['['];
      this.level += 1;
      for(var i=0, len=array.length; i<len; ++i) {
        // recursive calls need to allow for sub-classing
        json = ZOO.Format.JSON.prototype.write.apply(this,
                                                     [array[i], this.pretty]);
        if(json != null) {
          if(i > 0)
            pieces.push(',');
          pieces.push(this.writeNewline(), this.writeIndent(), json);
        }
      }
      this.level -= 1;    
      pieces.push(this.writeNewline(), this.writeIndent(), ']');
      return pieces.join('');
    },
    'string': function(string) {
      var m = {
                '\b': '\\b',
                '\t': '\\t',
                '\n': '\\n',
                '\f': '\\f',
                '\r': '\\r',
                '"' : '\\"',
                '\\': '\\\\'
      };
      if(/["\\\x00-\x1f]/.test(string)) {
        return '"' + string.replace(/([\x00-\x1f\\"])/g, function(a, b) {
            var c = m[b];
            if(c)
              return c;
            c = b.charCodeAt();
            return '\\u00' +
            Math.floor(c / 16).toString(16) +
            (c % 16).toString(16);
        }) + '"';
      }
      return '"' + string + '"';
    },
    'number': function(number) {
      return isFinite(number) ? String(number) : "null";
    },
    'boolean': function(bool) {
      return String(bool);
    },
    'date': function(date) {    
      function format(number) {
        // Format integers to have at least two digits.
        return (number < 10) ? '0' + number : number;
      }
      return '"' + date.getFullYear() + '-' +
        format(date.getMonth() + 1) + '-' +
        format(date.getDate()) + 'T' +
        format(date.getHours()) + ':' +
        format(date.getMinutes()) + ':' +
        format(date.getSeconds()) + '"';
    }
  },
  CLASS_NAME: 'ZOO.Format.JSON'
});
ZOO.Format.GeoJSON = ZOO.Class(ZOO.Format.JSON, {
  initialize: function(options) {
    ZOO.Format.JSON.prototype.initialize.apply(this, [options]);
  },
  read: function(json, type, filter) {
    type = (type) ? type : "FeatureCollection";
    var results = null;
    var obj = null;
    if (typeof json == "string")
      obj = ZOO.Format.JSON.prototype.read.apply(this,[json, filter]);
    else
      obj = json;
    if(!obj) {
      //OpenLayers.Console.error("Bad JSON: " + json);
    } else if(typeof(obj.type) != "string") {
      //OpenLayers.Console.error("Bad GeoJSON - no type: " + json);
    } else if(this.isValidType(obj, type)) {
      switch(type) {
        case "Geometry":
          try {
            results = this.parseGeometry(obj);
          } catch(err) {
            //OpenLayers.Console.error(err);
          }
          break;
        case "Feature":
          try {
            results = this.parseFeature(obj);
            results.type = "Feature";
          } catch(err) {
            //OpenLayers.Console.error(err);
          }
          break;
        case "FeatureCollection":
          // for type FeatureCollection, we allow input to be any type
          results = [];
          switch(obj.type) {
            case "Feature":
              try {
                results.push(this.parseFeature(obj));
              } catch(err) {
                results = null;
                //OpenLayers.Console.error(err);
              }
              break;
            case "FeatureCollection":
              for(var i=0, len=obj.features.length; i<len; ++i) {
                try {
                  results.push(this.parseFeature(obj.features[i]));
                } catch(err) {
                  results = null;
                  //OpenLayers.Console.error(err);
                }
              }
              break;
            default:
              try {
                var geom = this.parseGeometry(obj);
                results.push(new ZOO.Feature(geom));
              } catch(err) {
                results = null;
                //OpenLayers.Console.error(err);
              }
          }
          break;
      }
    }
    return results;
  },
  isValidType: function(obj, type) {
    var valid = false;
    switch(type) {
      case "Geometry":
        if(ZOO.indexOf(
              ["Point", "MultiPoint", "LineString", "MultiLineString",
              "Polygon", "MultiPolygon", "Box", "GeometryCollection"],
              obj.type) == -1) {
          // unsupported geometry type
          //OpenLayers.Console.error("Unsupported geometry type: " +obj.type);
        } else {
          valid = true;
        }
        break;
      case "FeatureCollection":
        // allow for any type to be converted to a feature collection
        valid = true;
        break;
      default:
        // for Feature types must match
        if(obj.type == type) {
          valid = true;
        } else {
          //OpenLayers.Console.error("Cannot convert types from " +obj.type + " to " + type);
        }
    }
    return valid;
  },
  parseFeature: function(obj) {
    var feature, geometry, attributes, bbox;
    attributes = (obj.properties) ? obj.properties : {};
    bbox = (obj.geometry && obj.geometry.bbox) || obj.bbox;
    try {
      geometry = this.parseGeometry(obj.geometry);
    } catch(err) {
      // deal with bad geometries
      throw err;
    }
    feature = new ZOO.Feature(geometry, attributes);
    if(bbox)
      feature.bounds = ZOO.Bounds.fromArray(bbox);
    if(obj.id)
      feature.fid = obj.id;
    return feature;
  },
  parseGeometry: function(obj) {
    if (obj == null)
      return null;
    var geometry, collection = false;
    if(obj.type == "GeometryCollection") {
      if(!(obj.geometries instanceof Array)) {
        throw "GeometryCollection must have geometries array: " + obj;
      }
      var numGeom = obj.geometries.length;
      var components = new Array(numGeom);
      for(var i=0; i<numGeom; ++i) {
        components[i] = this.parseGeometry.apply(
            this, [obj.geometries[i]]
            );
      }
      geometry = new ZOO.Geometry.Collection(components);
      collection = true;
    } else {
      if(!(obj.coordinates instanceof Array)) {
        throw "Geometry must have coordinates array: " + obj;
      }
      if(!this.parseCoords[obj.type.toLowerCase()]) {
        throw "Unsupported geometry type: " + obj.type;
      }
      try {
        geometry = this.parseCoords[obj.type.toLowerCase()].apply(
            this, [obj.coordinates]
            );
      } catch(err) {
        // deal with bad coordinates
        throw err;
      }
    }
        // We don't reproject collections because the children are reprojected
        // for us when they are created.
    if (this.internalProjection && this.externalProjection && !collection) {
      geometry.transform(this.externalProjection, 
          this.internalProjection); 
    }                       
    return geometry;
  },
  parseCoords: {
    "point": function(array) {
      if(array.length != 2) {
        throw "Only 2D points are supported: " + array;
      }
      return new ZOO.Geometry.Point(array[0], array[1]);
    },
    "multipoint": function(array) {
      var points = [];
      var p = null;
      for(var i=0, len=array.length; i<len; ++i) {
        try {
          p = this.parseCoords["point"].apply(this, [array[i]]);
        } catch(err) {
          throw err;
        }
        points.push(p);
      }
      return new ZOO.Geometry.MultiPoint(points);
    },
    "linestring": function(array) {
      var points = [];
      var p = null;
      for(var i=0, len=array.length; i<len; ++i) {
        try {
          p = this.parseCoords["point"].apply(this, [array[i]]);
        } catch(err) {
          throw err;
        }
        points.push(p);
      }
      return new ZOO.Geometry.LineString(points);
    },
    "multilinestring": function(array) {
      var lines = [];
      var l = null;
      for(var i=0, len=array.length; i<len; ++i) {
        try {
          l = this.parseCoords["linestring"].apply(this, [array[i]]);
        } catch(err) {
          throw err;
        }
        lines.push(l);
      }
      return new ZOO.Geometry.MultiLineString(lines);
    },
    "polygon": function(array) {
      var rings = [];
      var r, l;
      for(var i=0, len=array.length; i<len; ++i) {
        try {
          l = this.parseCoords["linestring"].apply(this, [array[i]]);
        } catch(err) {
          throw err;
        }
        r = new ZOO.Geometry.LinearRing(l.components);
        rings.push(r);
      }
      return new ZOO.Geometry.Polygon(rings);
    },
    "multipolygon": function(array) {
      var polys = [];
      var p = null;
      for(var i=0, len=array.length; i<len; ++i) {
        try {
          p = this.parseCoords["polygon"].apply(this, [array[i]]);
        } catch(err) {
          throw err;
        }
        polys.push(p);
      }
      return new ZOO.Geometry.MultiPolygon(polys);
    },
    "box": function(array) {
      if(array.length != 2) {
        throw "GeoJSON box coordinates must have 2 elements";
      }
      return new ZOO.Geometry.Polygon([
          new ZOO.Geometry.LinearRing([
            new ZOO.Geometry.Point(array[0][0], array[0][1]),
            new ZOO.Geometry.Point(array[1][0], array[0][1]),
            new ZOO.Geometry.Point(array[1][0], array[1][1]),
            new ZOO.Geometry.Point(array[0][0], array[1][1]),
            new Z0O.Geometry.Point(array[0][0], array[0][1])
          ])
      ]);
    }
  },
  write: function(obj, pretty) {
    var geojson = {
      "type": null
    };
    if(obj instanceof Array) {
      geojson.type = "FeatureCollection";
      var numFeatures = obj.length;
      geojson.features = new Array(numFeatures);
      for(var i=0; i<numFeatures; ++i) {
        var element = obj[i];
        if(!element instanceof ZOO.Feature) {
          var msg = "FeatureCollection only supports collections " +
            "of features: " + element;
          throw msg;
        }
        geojson.features[i] = this.extract.feature.apply(this, [element]);
      }
    } else if (obj.CLASS_NAME.indexOf("ZOO.Geometry") == 0) {
      geojson = this.extract.geometry.apply(this, [obj]);
    } else if (obj instanceof ZOO.Feature) {
      geojson = this.extract.feature.apply(this, [obj]);
      /*
      if(obj.layer && obj.layer.projection) {
        geojson.crs = this.createCRSObject(obj);
      }
      */
    }
    return ZOO.Format.JSON.prototype.write.apply(this,
                                                 [geojson, pretty]);
  },
  createCRSObject: function(object) {
    //var proj = object.layer.projection.toString();
    var proj = object.projection.toString();
    var crs = {};
    if (proj.match(/epsg:/i)) {
      var code = parseInt(proj.substring(proj.indexOf(":") + 1));
      if (code == 4326) {
        crs = {
          "type": "OGC",
          "properties": {
            "urn": "urn:ogc:def:crs:OGC:1.3:CRS84"
          }
        };
      } else {    
        crs = {
          "type": "EPSG",
          "properties": {
            "code": code 
          }
        };
      }    
    }
    return crs;
  },
  extract: {
    'feature': function(feature) {
      var geom = this.extract.geometry.apply(this, [feature.geometry]);
      return {
        "type": "Feature",
        "id": feature.fid == null ? feature.id : feature.fid,
        "properties": feature.attributes,
        "geometry": geom
      };
    },
    'geometry': function(geometry) {
      if (geometry == null)
        return null;
      if (this.internalProjection && this.externalProjection) {
        geometry = geometry.clone();
        geometry.transform(this.internalProjection, 
            this.externalProjection);
      }                       
      var geometryType = geometry.CLASS_NAME.split('.')[2];
      var data = this.extract[geometryType.toLowerCase()].apply(this, [geometry]);
      var json;
      if(geometryType == "Collection")
        json = {
          "type": "GeometryCollection",
          "geometries": data
        };
      else
        json = {
          "type": geometryType,
          "coordinates": data
        };
      return json;
    },
    'point': function(point) {
      return [point.x, point.y];
    },
    'multipoint': function(multipoint) {
      var array = [];
      for(var i=0, len=multipoint.components.length; i<len; ++i) {
        array.push(this.extract.point.apply(this, [multipoint.components[i]]));
      }
      return array;
    },
    'linestring': function(linestring) {
      var array = [];
      for(var i=0, len=linestring.components.length; i<len; ++i) {
        array.push(this.extract.point.apply(this, [linestring.components[i]]));
      }
      return array;
    },
    'multilinestring': function(multilinestring) {
      var array = [];
      for(var i=0, len=multilinestring.components.length; i<len; ++i) {
        array.push(this.extract.linestring.apply(this, [multilinestring.components[i]]));
      }
      return array;
    },
    'polygon': function(polygon) {
      var array = [];
      for(var i=0, len=polygon.components.length; i<len; ++i) {
        array.push(this.extract.linestring.apply(this, [polygon.components[i]]));
      }
      return array;
    },
    'multipolygon': function(multipolygon) {
      var array = [];
      for(var i=0, len=multipolygon.components.length; i<len; ++i) {
        array.push(this.extract.polygon.apply(this, [multipolygon.components[i]]));
      }
      return array;
    },
    'collection': function(collection) {
      var len = collection.components.length;
      var array = new Array(len);
      for(var i=0; i<len; ++i) {
        array[i] = this.extract.geometry.apply(
            this, [collection.components[i]]
            );
      }
      return array;
    }
  },
  CLASS_NAME: 'ZOO.Format.GeoJSON'
});
ZOO.Format.KML = ZOO.Class(ZOO.Format, {
  kmlns: "http://www.opengis.net/kml/2.2",
  foldersName: "ZOO export",
  foldersDesc: "Created on " + new Date(),
  placemarksDesc: "No description available",
  extractAttributes: true,
  initialize: function(options) {
    // compile regular expressions once instead of every time they are used
    this.regExes = {
           trimSpace: (/^\s*|\s*$/g),
           removeSpace: (/\s*/g),
           splitSpace: (/\s+/),
           trimComma: (/\s*,\s*/g),
           kmlColor: (/(\w{2})(\w{2})(\w{2})(\w{2})/),
           kmlIconPalette: (/root:\/\/icons\/palette-(\d+)(\.\w+)/),
           straightBracket: (/\$\[(.*?)\]/g)
    };
    ZOO.Format.prototype.initialize.apply(this, [options]);
  },
  read: function(data) {
    this.features = [];
    data = data.replace(/^<\?xml\s+version\s*=\s*(["'])[^\1]+\1[^?]*\?>/, "");
    data = new XML(data);
    var placemarks = data..*::Placemark;
    this.parseFeatures(placemarks);
    return this.features;
  },
  parseFeatures: function(nodes) {
    var features = new Array(nodes.length());
    for(var i=0, len=nodes.length(); i<len; i++) {
      var featureNode = nodes[i];
      var feature = this.parseFeature.apply(this,[featureNode]) ;
      features[i] = feature;
    }
    this.features = this.features.concat(features);
  },
  parseFeature: function(node) {
    var order = ["MultiGeometry", "Polygon", "LineString", "Point"];
    var type, nodeList, geometry, parser;
    for(var i=0, len=order.length; i<len; ++i) {
      type = order[i];
      nodeList = node.descendants(QName(null,type));
      if (nodeList.length()> 0) {
        var parser = this.parseGeometry[type.toLowerCase()];
        if(parser) {
          geometry = parser.apply(this, [nodeList[0]]);
          if (this.internalProjection && this.externalProjection) {
            geometry.transform(this.externalProjection, 
                               this.internalProjection); 
          }                       
        }
        break;
      }
    }
    var attributes;
    if(this.extractAttributes) {
      attributes = this.parseAttributes(node);
    }
    var feature = new ZOO.Feature(geometry, attributes);
    var fid = node.@id || node.@name;
    if(fid != null)
      feature.fid = fid;
    return feature;
  },
  parseGeometry: {
    'point': function(node) {
      var coordString = node.*::coordinates.toString();
      coordString = coordString.replace(this.regExes.removeSpace, "");
      coords = coordString.split(",");
      var point = null;
      if(coords.length > 1) {
        // preserve third dimension
        if(coords.length == 2) {
          coords[2] = null;
        }
        point = new ZOO.Geometry.Point(coords[0], coords[1], coords[2]);
      }
      return point;
    },
    'linestring': function(node, ring) {
      var line = null;
      var coordString = node.*::coordinates.toString();
      coordString = coordString.replace(this.regExes.trimSpace,
          "");
      coordString = coordString.replace(this.regExes.trimComma,
          ",");
      var pointList = coordString.split(this.regExes.splitSpace);
      var numPoints = pointList.length;
      var points = new Array(numPoints);
      var coords, numCoords;
      for(var i=0; i<numPoints; ++i) {
        coords = pointList[i].split(",");
        numCoords = coords.length;
        if(numCoords > 1) {
          if(coords.length == 2) {
            coords[2] = null;
          }
          points[i] = new ZOO.Geometry.Point(coords[0],
                                             coords[1],
                                             coords[2]);
        }
      }
      if(numPoints) {
        if(ring) {
          line = new ZOO.Geometry.LinearRing(points);
        } else {
          line = new ZOO.Geometry.LineString(points);
        }
      } else {
        throw "Bad LineString coordinates: " + coordString;
      }
      return line;
    },
    'polygon': function(node) {
      var nodeList = node..*::LinearRing;
      var numRings = nodeList.length();
      var components = new Array(numRings);
      if(numRings > 0) {
        // this assumes exterior ring first, inner rings after
        var ring;
        for(var i=0, len=nodeList.length(); i<len; ++i) {
          ring = this.parseGeometry.linestring.apply(this,
                                                     [nodeList[i], true]);
          if(ring) {
            components[i] = ring;
          } else {
            throw "Bad LinearRing geometry: " + i;
          }
        }
      }
      return new ZOO.Geometry.Polygon(components);
    },
    multigeometry: function(node) {
      var child, parser;
      var parts = [];
      var children = node.*::*;
      for(var i=0, len=children.length(); i<len; ++i ) {
        child = children[i];
        var type = child.localName();
        var parser = this.parseGeometry[type.toLowerCase()];
        if(parser) {
          parts.push(parser.apply(this, [child]));
        }
      }
      return new ZOO.Geometry.Collection(parts);
    }
  },
  parseAttributes: function(node) {
    var attributes = {};
    var edNodes = node.*::ExtendedData;
    if (edNodes.length() > 0) {
      attributes = this.parseExtendedData(edNodes[0])
    }
    var child, grandchildren;
    var children = node.*::*;
    for(var i=0, len=children.length(); i<len; ++i) {
      child = children[i];
      grandchildren = child..*::*;
      if(grandchildren.length() == 1) {
        var name = child.localName();
        var value = child.toString();
        if (value) {
          value = value.replace(this.regExes.trimSpace, "");
          attributes[name] = value;
        }
      }
    }
    return attributes;
  },
  parseExtendedData: function(node) {
    var attributes = {};
    var dataNodes = node.*::Data;
    for (var i = 0, len = dataNodes.length(); i < len; i++) {
      var data = dataNodes[i];
      var key = data.@name;
      var ed = {};
      var valueNode = data.*::value;
      if (valueNode.length() > 0)
        ed['value'] = valueNode[0].toString();
      var nameNode = data.*::displayName;
      if (nameNode.length() > 0)
        ed['displayName'] = valueNode[0].toString();
      attributes[key] = ed;
    }
    return attributes;
  },
  write: function(features) {
    if(!(features instanceof Array))
      features = [features];
    var kml = new XML('<kml xmlns="'+this.kmlns+'"></kml>');
    var folder = kml.Document.Folder;
    folder.name = this.foldersName;
    folder.description = this.foldersDesc;
    for(var i=0, len=features.length; i<len; ++i) {
      //folder.appendChild(this.createPlacemarkXML(features[i]));
      folder.Placemark[i] = this.createPlacemark(features[i]);
    }
    return kml.toXMLString();
  },
  createPlacemark: function(feature) {
    var placemark = new XML('<Placemark xmlns="'+this.kmlns+'"></Placemark>');
    placemark.name = (feature.attributes.name) ?
                    feature.attributes.name : feature.id;
    placemark.description = (feature.attributes.description) ?
                             feature.attributes.description : this.placemarksDesc;
    if(feature.fid != null)
      placemark.@id = feature.fid;
    placemark.*[2] = this.buildGeometryNode(feature.geometry);
    return placemark;
  },
  buildGeometryNode: function(geometry) {
    if (this.internalProjection && this.externalProjection) {
      geometry = geometry.clone();
      geometry.transform(this.internalProjection, 
                         this.externalProjection);
    }
    var className = geometry.CLASS_NAME;
    var type = className.substring(className.lastIndexOf(".") + 1);
    var builder = this.buildGeometry[type.toLowerCase()];
    var node = null;
    if(builder) {
      node = builder.apply(this, [geometry]);
    }
    return node;
  },
  buildGeometry: {
    'point': function(geometry) {
      var kml = new XML('<Point xmlns="'+this.kmlns+'"></Point>');
      kml.coordinates = this.buildCoordinatesNode(geometry);
      return kml;
    },
    'multipoint': function(geometry) {
      return this.buildGeometry.collection.apply(this, [geometry]);
    },
    'linestring': function(geometry) {
      var kml = new XML('<LineString xmlns="'+this.kmlns+'"></LineString>');
      kml.coordinates = this.buildCoordinatesNode(geometry);
      return kml;
    },
    'multilinestring': function(geometry) {
      return this.buildGeometry.collection.apply(this, [geometry]);
    },
    'linearring': function(geometry) {
      var kml = new XML('<LinearRing xmlns="'+this.kmlns+'"></LinearRing>');
      kml.coordinates = this.buildCoordinatesNode(geometry);
      return kml;
    },
    'polygon': function(geometry) {
      var kml = new XML('<Polygon xmlns="'+this.kmlns+'"></Polygon>');
      var rings = geometry.components;
      var ringMember, ringGeom, type;
      for(var i=0, len=rings.length; i<len; ++i) {
        type = (i==0) ? "outerBoundaryIs" : "innerBoundaryIs";
        ringMember = new XML('<'+type+' xmlns="'+this.kmlns+'"></'+type+'>');
        ringMember.LinearRing = this.buildGeometry.linearring.apply(this,[rings[i]]);
        kml.*[i] = ringMember;
      }
      return kml;
    },
    'multipolygon': function(geometry) {
      return this.buildGeometry.collection.apply(this, [geometry]);
    },
    'collection': function(geometry) {
      var kml = new XML('<MultiGeometry xmlns="'+this.kmlns+'"></MultiGeometry>');
      var child;
      for(var i=0, len=geometry.components.length; i<len; ++i) {
        kml.*[i] = this.buildGeometryNode.apply(this,[geometry.components[i]]);
      }
      return kml;
    }
  },
  buildCoordinatesNode: function(geometry) {
    var cooridnates = new XML('<coordinates xmlns="'+this.kmlns+'"></coordinates>');
    var points = geometry.components;
    if(points) {
      // LineString or LinearRing
      var point;
      var numPoints = points.length;
      var parts = new Array(numPoints);
      for(var i=0; i<numPoints; ++i) {
        point = points[i];
        parts[i] = point.x + "," + point.y;
      }
      coordinates = parts.join(" ");
    } else {
      // Point
      coordinates = geometry.x + "," + geometry.y;
    }
    return coordinates;
  },
  CLASS_NAME: 'ZOO.Format.KML'
});
ZOO.Format.GML = ZOO.Class(ZOO.Format, {
  schemaLocation: "http://www.opengis.net/gml http://schemas.opengis.net/gml/2.1.2/feature.xsd",
  namespaces: {
    ogr: "http://ogr.maptools.org/",
    gml: "http://www.opengis.net/gml",
    xlink: "http://www.w3.org/1999/xlink",
    xsi: "http://www.w3.org/2001/XMLSchema-instance",
    wfs: "http://www.opengis.net/wfs" // this is a convenience for reading wfs:FeatureCollection
  },
  defaultPrefix: 'ogr',
  collectionName: "FeatureCollection",
  featureName: "sql_statement",
  geometryName: "geometryProperty",
  xy: true,
  initialize: function(options) {
    // compile regular expressions once instead of every time they are used
    this.regExes = {
      trimSpace: (/^\s*|\s*$/g),
      removeSpace: (/\s*/g),
      splitSpace: (/\s+/),
      trimComma: (/\s*,\s*/g)
    };
    ZOO.Format.prototype.initialize.apply(this, [options]);
  },
  read: function(data) {
    this.features = [];
    data = data.replace(/^<\?xml\s+version\s*=\s*(["'])[^\1]+\1[^?]*\?>/, "");
    data = new XML(data);

    var gmlns = Namespace(this.namespaces['gml']);
    var featureNodes = data..gmlns::featureMember;
    var features = [];
    for(var i=0,len=featureNodes.length(); i<len; i++) {
      var feature = this.parseFeature(featureNodes[i]);
      if(feature) {
        features.push(feature);
      }
    }
    return features;
  },
  parseFeature: function(node) {
    // only accept one geometry per feature - look for highest "order"
    var gmlns = Namespace(this.namespaces['gml']);
    var order = ["MultiPolygon", "Polygon",
                 "MultiLineString", "LineString",
                 "MultiPoint", "Point", "Envelope", "Box"];
    var type, nodeList, geometry, parser;
    for(var i=0; i<order.length; ++i) {
      type = order[i];
      nodeList = node.descendants(QName(gmlns,type));
      if (nodeList.length() > 0) {
        var parser = this.parseGeometry[type.toLowerCase()];
        if(parser) {
          geometry = parser.apply(this, [nodeList[0]]);
          if (this.internalProjection && this.externalProjection) {
            geometry.transform(this.externalProjection, 
                               this.internalProjection); 
          }                       
        }
        break;
      }
    }
    var attributes;
    if(this.extractAttributes) {
      //attributes = this.parseAttributes(node);
    }
    var feature = new ZOO.Feature(geometry, attributes);
    return feature;
  },
  parseGeometry: {
    'point': function(node) {
      /**
       * Three coordinate variations to consider:
       * 1) <gml:pos>x y z</gml:pos>
       * 2) <gml:coordinates>x, y, z</gml:coordinates>
       * 3) <gml:coord><gml:X>x</gml:X><gml:Y>y</gml:Y></gml:coord>
       */
      var nodeList, coordString;
      var coords = [];
      // look for <gml:pos>
      var nodeList = node..*::pos;
      if(nodeList.length() > 0) {
        coordString = nodeList[0].toString();
        coordString = coordString.replace(this.regExes.trimSpace, "");
        coords = coordString.split(this.regExes.splitSpace);
      }
      // look for <gml:coordinates>
      if(coords.length == 0) {
        nodeList = node..*::coordinates;
        if(nodeList.length() > 0) {
          coordString = nodeList[0].toString();
          coordString = coordString.replace(this.regExes.removeSpace,"");
          coords = coordString.split(",");
        }
      }
      // look for <gml:coord>
      if(coords.length == 0) {
        nodeList = node..*::coord;
        if(nodeList.length() > 0) {
          var xList = nodeList[0].*::X;
          var yList = nodeList[0].*::Y;
          if(xList.length() > 0 && yList.length() > 0)
            coords = [xList[0].toString(),
                      yList[0].toString()];
        }
      }
      // preserve third dimension
      if(coords.length == 2)
        coords[2] = null;
      if (this.xy)
        return new ZOO.Geometry.Point(coords[0],coords[1],coords[2]);
      else
        return new ZOO.Geometry.Point(coords[1],coords[0],coords[2]);
    },
    'multipoint': function(node) {
      var nodeList = node..*::Point;
      var components = [];
      if(nodeList.length() > 0) {
        var point;
        for(var i=0, len=nodeList.length(); i<len; ++i) {
          point = this.parseGeometry.point.apply(this, [nodeList[i]]);
          if(point)
            components.push(point);
        }
      }
      return new ZOO.Geometry.MultiPoint(components);
    },
    'linestring': function(node, ring) {
      /**
       * Two coordinate variations to consider:
       * 1) <gml:posList dimension="d">x0 y0 z0 x1 y1 z1</gml:posList>
       * 2) <gml:coordinates>x0, y0, z0 x1, y1, z1</gml:coordinates>
       */
      var nodeList, coordString;
      var coords = [];
      var points = [];
      // look for <gml:posList>
      nodeList = node..*::posList;
      if(nodeList.length() > 0) {
        coordString = nodeList[0].toString();
        coordString = coordString.replace(this.regExes.trimSpace, "");
        coords = coordString.split(this.regExes.splitSpace);
        var dim = parseInt(nodeList[0].@dimension);
        var j, x, y, z;
        for(var i=0; i<coords.length/dim; ++i) {
          j = i * dim;
          x = coords[j];
          y = coords[j+1];
          z = (dim == 2) ? null : coords[j+2];
          if (this.xy)
            points.push(new ZOO.Geometry.Point(x, y, z));
          else
            points.push(new Z0O.Geometry.Point(y, x, z));
        }
      }
      // look for <gml:coordinates>
      if(coords.length == 0) {
        nodeList = node..*::coordinates;
        if(nodeList.length() > 0) {
          coordString = nodeList[0].toString();
          coordString = coordString.replace(this.regExes.trimSpace,"");
          coordString = coordString.replace(this.regExes.trimComma,",");
          var pointList = coordString.split(this.regExes.splitSpace);
          for(var i=0; i<pointList.length; ++i) {
            coords = pointList[i].split(",");
            if(coords.length == 2)
              coords[2] = null;
            if (this.xy)
              points.push(new ZOO.Geometry.Point(coords[0],coords[1],coords[2]));
            else
              points.push(new ZOO.Geometry.Point(coords[1],coords[0],coords[2]));
          }
        }
      }
      var line = null;
      if(points.length != 0) {
        if(ring)
          line = new ZOO.Geometry.LinearRing(points);
        else
          line = new ZOO.Geometry.LineString(points);
      }
      return line;
    },
    'multilinestring': function(node) {
      var nodeList = node..*::LineString;
      var components = [];
      if(nodeList.length() > 0) {
        var line;
        for(var i=0, len=nodeList.length(); i<len; ++i) {
          line = this.parseGeometry.linestring.apply(this, [nodeList[i]]);
          if(point)
            components.push(point);
        }
      }
      return new ZOO.Geometry.MultiLineString(components);
    },
    'polygon': function(node) {
      nodeList = node..*::LinearRing;
      var components = [];
      if(nodeList.length() > 0) {
        // this assumes exterior ring first, inner rings after
        var ring;
        for(var i=0, len = nodeList.length(); i<len; ++i) {
          ring = this.parseGeometry.linestring.apply(this,[nodeList[i], true]);
          if(ring)
            components.push(ring);
        }
      }
      return new ZOO.Geometry.Polygon(components);
    },
    'multipolygon': function(node) {
      var nodeList = node..*::Polygon;
      var components = [];
      if(nodeList.length() > 0) {
        var polygon;
        for(var i=0, len=nodeList.length(); i<len; ++i) {
          polygon = this.parseGeometry.polygon.apply(this, [nodeList[i]]);
          if(polygon)
            components.push(polygon);
        }
      }
      return new ZOO.Geometry.MultiPolygon(components);
    },
    'envelope': function(node) {
      var components = [];
      var coordString;
      var envelope;
      var lpoint = node..*::lowerCorner;
      if (lpoint.length() > 0) {
        var coords = [];
        if(lpoint.length() > 0) {
          coordString = lpoint[0].toString();
          coordString = coordString.replace(this.regExes.trimSpace, "");
          coords = coordString.split(this.regExes.splitSpace);
        }
        if(coords.length == 2)
          coords[2] = null;
        if (this.xy)
          var lowerPoint = new ZOO.Geometry.Point(coords[0], coords[1],coords[2]);
        else
          var lowerPoint = new ZOO.Geometry.Point(coords[1], coords[0],coords[2]);
      }
      var upoint = node..*::upperCorner;
      if (upoint.length() > 0) {
        var coords = [];
        if(upoint.length > 0) {
          coordString = upoint[0].toString();
          coordString = coordString.replace(this.regExes.trimSpace, "");
          coords = coordString.split(this.regExes.splitSpace);
        }
        if(coords.length == 2)
          coords[2] = null;
        if (this.xy)
          var upperPoint = new ZOO.Geometry.Point(coords[0], coords[1],coords[2]);
        else
          var upperPoint = new ZOO.Geometry.Point(coords[1], coords[0],coords[2]);
      }
      if (lowerPoint && upperPoint) {
        components.push(new ZOO.Geometry.Point(lowerPoint.x, lowerPoint.y));
        components.push(new ZOO.Geometry.Point(upperPoint.x, lowerPoint.y));
        components.push(new ZOO.Geometry.Point(upperPoint.x, upperPoint.y));
        components.push(new ZOO.Geometry.Point(lowerPoint.x, upperPoint.y));
        components.push(new ZOO.Geometry.Point(lowerPoint.x, lowerPoint.y));
        var ring = new ZOO.Geometry.LinearRing(components);
        envelope = new ZOO.Geometry.Polygon([ring]);
      }
      return envelope;
    }
  },
  write: function(features) {
    if(!(features instanceof Array)) {
      features = [features];
    }
    var pfx = this.defaultPrefix;
    var name = pfx+':'+this.collectionName;
    var gml = new XML('<'+name+' xmlns:'+pfx+'="'+this.namespaces[pfx]+'" xmlns:gml="'+this.namespaces['gml']+'" xmlns:xsi="'+this.namespaces['xsi']+'" xsi:schemaLocation="'+this.schemaLocation+'"></'+name+'>');
    for(var i=0; i<features.length; i++) {
      gml.*::*[i] = this.createFeatureXML(features[i]);
    }
    return gml.toXMLString();
  },
  createFeatureXML: function(feature) {
    var pfx = this.defaultPrefix;
    var name = pfx+':'+this.featureName;
    var fid = feature.fid || feature.id;
    var gml = new XML('<gml:featureMember xmlns:gml="'+this.namespaces['gml']+'"><'+name+' xmlns:'+pfx+'="'+this.namespaces[pfx]+'" fid="'+fid+'"></'+name+'></gml:featureMember>');
    var geometry = feature.geometry;
    gml.*::*[0].*::* = this.buildGeometryNode(geometry);
    for(var attr in feature.attributes) {
      var attrNode = new XML('<'+pfx+':'+attr+' xmlns:'+pfx+'="'+this.namespaces[pfx]+'">'+feature.attributes[attr]+'</'+pfx+':'+attr+'>');
      gml.*::*[0].appendChild(attrNode);
    }
    return gml;
  },
  buildGeometryNode: function(geometry) {
    if (this.externalProjection && this.internalProjection) {
      geometry = geometry.clone();
      geometry.transform(this.internalProjection, 
          this.externalProjection);
    }    
    var className = geometry.CLASS_NAME;
    var type = className.substring(className.lastIndexOf(".") + 1);
    var builder = this.buildGeometry[type.toLowerCase()];
    var pfx = this.defaultPrefix;
    var name = pfx+':'+this.geometryName;
    var gml = new XML('<'+name+' xmlns:'+pfx+'="'+this.namespaces[pfx]+'"></'+name+'>');
    if (builder)
      gml.*::* = builder.apply(this, [geometry]);
    return gml;
  },
  buildGeometry: {
    'point': function(geometry) {
      var gml = new XML('<gml:Point xmlns:gml="'+this.namespaces['gml']+'"></gml:Point>');
      gml.*::*[0] = this.buildCoordinatesNode(geometry);
      return gml;
    },
    'multipoint': function(geometry) {
      var gml = new XML('<gml:MultiPoint xmlns:gml="'+this.namespaces['gml']+'"></gml:MultiPoint>');
      var points = geometry.components;
      var pointMember;
      for(var i=0; i<points.length; i++) { 
        pointMember = new XML('<gml:pointMember xmlns:gml="'+this.namespaces['gml']+'"></gml:pointMember>');
        pointMember.*::* = this.buildGeometry.point.apply(this,[points[i]]);
        gml.*::*[i] = pointMember;
      }
      return gml;            
    },
    'linestring': function(geometry) {
      var gml = new XML('<gml:LineString xmlns:gml="'+this.namespaces['gml']+'"></gml:LineString>');
      gml.*::*[0] = this.buildCoordinatesNode(geometry);
      return gml;
    },
    'multilinestring': function(geometry) {
      var gml = new XML('<gml:MultiLineString xmlns:gml="'+this.namespaces['gml']+'"></gml:MultiLineString>');
      var lines = geometry.components;
      var lineMember;
      for(var i=0; i<lines.length; i++) { 
        lineMember = new XML('<gml:lineStringMember xmlns:gml="'+this.namespaces['gml']+'"></gml:lineStringMember>');
        lineMember.*::* = this.buildGeometry.linestring.apply(this,[lines[i]]);
        gml.*::*[i] = lineMember;
      }
      return gml;            
    },
    'linearring': function(geometry) {
      var gml = new XML('<gml:LinearRing xmlns:gml="'+this.namespaces['gml']+'"></gml:LinearRing>');
      gml.*::*[0] = this.buildCoordinatesNode(geometry);
      return gml;
    },
    'polygon': function(geometry) {
      var gml = new XML('<gml:Polygon xmlns:gml="'+this.namespaces['gml']+'"></gml:Polygon>');
      var rings = geometry.components;
      var ringMember, type;
      for(var i=0; i<rings.length; ++i) {
        type = (i==0) ? "outerBoundaryIs" : "innerBoundaryIs";
        var ringMember = new XML('<gml:'+type+' xmlns:gml="'+this.namespaces['gml']+'"></gml:'+type+'>');
        ringMember.*::* = this.buildGeometry.linearring.apply(this,[rings[i]]);
        gml.*::*[i] = ringMember;
      }
      return gml;
    },
    'multipolygon': function(geometry) {
      var gml = new XML('<gml:MultiPolygon xmlns:gml="'+this.namespaces['gml']+'"></gml:MultiPolygon>');
      var polys = geometry.components;
      var polyMember;
      for(var i=0; i<polys.length; i++) { 
        polyMember = new XML('<gml:polygonMember xmlns:gml="'+this.namespaces['gml']+'"></gml:polygonMember>');
        polyMember.*::* = this.buildGeometry.polygon.apply(this,[polys[i]]);
        gml.*::*[i] = polyMember;
      }
      return gml;            
    },
    'bounds': function(bounds) {
      var gml = new XML('<gml:Box xmlns:gml="'+this.namespaces['gml']+'"></gml:Box>');
      gml.*::*[0] = this.buildCoordinatesNode(bounds);
      return gml;
    }
  },
  buildCoordinatesNode: function(geometry) {
    var parts = [];
    if(geometry instanceof ZOO.Bounds){
      parts.push(geometry.left + "," + geometry.bottom);
      parts.push(geometry.right + "," + geometry.top);
    } else {
      var points = (geometry.components) ? geometry.components : [geometry];
      for(var i=0; i<points.length; i++) {
        parts.push(points[i].x + "," + points[i].y);                
      }            
    }
    return new XML('<gml:coordinates xmlns:gml="'+this.namespaces['gml']+'" decimal="." cs=", " ts=" ">'+parts.join(" ")+'</gml:coordinates>');
  },
  CLASS_NAME: 'ZOO.Format.GML'
});
ZOO.Format.WPS = ZOO.Class(ZOO.Format, {
  schemaLocation: "http://www.opengis.net/wps/1.0.0/../wpsExecute_request.xsd",
  namespaces: {
    ows: "http://www.opengis.net/ows/1.1",
    wps: "http://www.opengis.net/wps/1.0.0",
    xlink: "http://www.w3.org/1999/xlink",
    xsi: "http://www.w3.org/2001/XMLSchema-instance",
  },
  read:function(data) {
    data = data.replace(/^<\?xml\s+version\s*=\s*(["'])[^\1]+\1[^?]*\?>/, "");
    data = new XML(data);
    switch (data.localName()) {
      case 'ExecuteResponse':
        return this.parseExecuteResponse(data);
      default:
        return null;
    }
  },
  parseExecuteResponse: function(node) {
    var outputs = node.*::ProcessOutputs.*::Output;
    if (outputs.length() > 0) {
      var data = outputs[0].*::Data.*::*[0];
      var builder = this.parseData[data.localName().toLowerCase()];
      if (builder)
        return builder.apply(this,[data]);
      else
        return null;
    } else
      return null;
  },
  parseData: {
    'complexdata': function(node) {
      var result = {value:node.toString()};
      if (node.@mimeType.length()>0)
        result.mimeType = node.@mimeType;
      if (node.@encoding.length()>0)
        result.encoding = node.@encoding;
      if (node.@schema.length()>0)
        result.schema = node.@schema;
      return result;
    },
    'literaldata': function(node) {
      var result = {value:node.toString()};
      if (node.@dataType.length()>0)
        result.dataType = node.@dataType;
      if (node.@uom.length()>0)
        result.uom = node.@uom;
      return result;
    }
  },
  CLASS_NAME: 'ZOO.Format.WPS'
});


ZOO.Feature = ZOO.Class({
  fid: null,
  geometry: null,
  attributes: null,
  bounds: null,
  initialize: function(geometry, attributes) {
    this.geometry = geometry ? geometry : null;
    this.attributes = {};
    if (attributes)
      this.attributes = ZOO.extend(this.attributes,attributes);
  },
  destroy: function() {
    this.geometry = null;
  },
  clone: function () {
    return new ZOO.Feature(this.geometry ? this.geometry.clone() : null,
            this.attributes);
  },
  move: function(x, y) {
    if(!this.geometry.move)
      return;

    this.geometry.move(x,y);
    return this.geometry;
  },
  CLASS_NAME: 'ZOO.Feature'
});

ZOO.Geometry = ZOO.Class({
  id: null,
  parent: null,
  bounds: null,
  initialize: function() {
    //generate unique id
  },
  destroy: function() {
    this.id = null;
    this.bounds = null;
  },
  clone: function() {
    return new ZOO.Geometry();
  },
  extendBounds: function(newBounds){
    var bounds = this.getBounds();
    if (!bounds)
      this.setBounds(newBounds);
    else
      this.bounds.extend(newBounds);
  },
  setBounds: function(bounds) {
    if (bounds)
      this.bounds = bounds.clone();
  },
  clearBounds: function() {
    this.bounds = null;
    if (this.parent)
      this.parent.clearBounds();
  },
  getBounds: function() {
    if (this.bounds == null) {
      this.calculateBounds();
    }
    return this.bounds;
  },
  calculateBounds: function() {
    return this.bounds = null;
  },
  distanceTo: function(geometry, options) {
  },
  getVertices: function(nodes) {
  },
  getLength: function() {
    return 0.0;
  },
  getArea: function() {
    return 0.0;
  },
  getCentroid: function() {
    return null;
  },
  toString: function() {
    return ZOO.Format.WKT.prototype.write(
        new ZOO.Feature(this)
    );
  },
  CLASS_NAME: 'ZOO.Geometry'
});
ZOO.Geometry.fromWKT = function(wkt) {
  var format = arguments.callee.format;
  if(!format) {
    format = new ZOO.Format.WKT();
    arguments.callee.format = format;
  }
  var geom;
  var result = format.read(wkt);
  if(result instanceof ZOO.Feature) {
    geom = result.geometry;
  } else if(result instanceof Array) {
    var len = result.length;
    var components = new Array(len);
    for(var i=0; i<len; ++i) {
      components[i] = result[i].geometry;
    }
    geom = new ZOO.Geometry.Collection(components);
  }
  return geom;
};
ZOO.Geometry.segmentsIntersect = function(seg1, seg2, options) {
  var point = options && options.point;
  var tolerance = options && options.tolerance;
  var intersection = false;
  var x11_21 = seg1.x1 - seg2.x1;
  var y11_21 = seg1.y1 - seg2.y1;
  var x12_11 = seg1.x2 - seg1.x1;
  var y12_11 = seg1.y2 - seg1.y1;
  var y22_21 = seg2.y2 - seg2.y1;
  var x22_21 = seg2.x2 - seg2.x1;
  var d = (y22_21 * x12_11) - (x22_21 * y12_11);
  var n1 = (x22_21 * y11_21) - (y22_21 * x11_21);
  var n2 = (x12_11 * y11_21) - (y12_11 * x11_21);
  if(d == 0) {
    // parallel
    if(n1 == 0 && n2 == 0) {
      // coincident
      intersection = true;
    }
  } else {
    var along1 = n1 / d;
    var along2 = n2 / d;
    if(along1 >= 0 && along1 <= 1 && along2 >=0 && along2 <= 1) {
      // intersect
      if(!point) {
        intersection = true;
      } else {
        // calculate the intersection point
        var x = seg1.x1 + (along1 * x12_11);
        var y = seg1.y1 + (along1 * y12_11);
        intersection = new ZOO.Geometry.Point(x, y);
      }
    }
  }
  if(tolerance) {
    var dist;
    if(intersection) {
      if(point) {
        var segs = [seg1, seg2];
        var seg, x, y;
        // check segment endpoints for proximity to intersection
        // set intersection to first endpoint within the tolerance
        outer: for(var i=0; i<2; ++i) {
          seg = segs[i];
          for(var j=1; j<3; ++j) {
            x = seg["x" + j];
            y = seg["y" + j];
            dist = Math.sqrt(
                Math.pow(x - intersection.x, 2) +
                Math.pow(y - intersection.y, 2)
            );
            if(dist < tolerance) {
              intersection.x = x;
              intersection.y = y;
              break outer;
            }
          }
        }
      }
    } else {
      // no calculated intersection, but segments could be within
      // the tolerance of one another
      var segs = [seg1, seg2];
      var source, target, x, y, p, result;
      // check segment endpoints for proximity to intersection
      // set intersection to first endpoint within the tolerance
      outer: for(var i=0; i<2; ++i) {
        source = segs[i];
        target = segs[(i+1)%2];
        for(var j=1; j<3; ++j) {
          p = {x: source["x"+j], y: source["y"+j]};
          result = ZOO.Geometry.distanceToSegment(p, target);
          if(result.distance < tolerance) {
            if(point) {
              intersection = new ZOO.Geometry.Point(p.x, p.y);
            } else {
              intersection = true;
            }
            break outer;
          }
        }
      }
    }
  }
  return intersection;
};
ZOO.Geometry.distanceToSegment = function(point, segment) {
  var x0 = point.x;
  var y0 = point.y;
  var x1 = segment.x1;
  var y1 = segment.y1;
  var x2 = segment.x2;
  var y2 = segment.y2;
  var dx = x2 - x1;
  var dy = y2 - y1;
  var along = ((dx * (x0 - x1)) + (dy * (y0 - y1))) /
               (Math.pow(dx, 2) + Math.pow(dy, 2));
  var x, y;
  if(along <= 0.0) {
    x = x1;
    y = y1;
  } else if(along >= 1.0) {
    x = x2;
    y = y2;
  } else {
    x = x1 + along * dx;
    y = y1 + along * dy;
  }
  return {
    distance: Math.sqrt(Math.pow(x - x0, 2) + Math.pow(y - y0, 2)),
    x: x, y: y
  };
};
ZOO.Geometry.Collection = ZOO.Class(ZOO.Geometry, {
  components: null,
  componentTypes: null,
  initialize: function (components) {
    ZOO.Geometry.prototype.initialize.apply(this, arguments);
    this.components = [];
    if (components != null) {
      this.addComponents(components);
    }
  },
  destroy: function () {
    this.components.length = 0;
    this.components = null;
  },
  clone: function() {
    var geometry = eval("new " + this.CLASS_NAME + "()");
    for(var i=0, len=this.components.length; i<len; i++) {
      geometry.addComponent(this.components[i].clone());
    }
    return geometry;
  },
  getComponentsString: function(){
    var strings = [];
    for(var i=0, len=this.components.length; i<len; i++) {
      strings.push(this.components[i].toShortString()); 
    }
    return strings.join(",");
  },
  calculateBounds: function() {
    this.bounds = null;
    if ( this.components && this.components.length > 0) {
      this.setBounds(this.components[0].getBounds());
      for (var i=1, len=this.components.length; i<len; i++) {
        this.extendBounds(this.components[i].getBounds());
      }
    }
    return this.bounds
  },
  addComponents: function(components){
    if(!(components instanceof Array))
      components = [components];
    for(var i=0, len=components.length; i<len; i++) {
      this.addComponent(components[i]);
    }
  },
  addComponent: function(component, index) {
    var added = false;
    if(component) {
      if(this.componentTypes == null ||
          (ZOO.indexOf(this.componentTypes,
                       component.CLASS_NAME) > -1)) {
        if(index != null && (index < this.components.length)) {
          var components1 = this.components.slice(0, index);
          var components2 = this.components.slice(index, 
                                                  this.components.length);
          components1.push(component);
          this.components = components1.concat(components2);
        } else {
          this.components.push(component);
        }
        component.parent = this;
        this.clearBounds();
        added = true;
      }
    }
    return added;
  },
  removeComponents: function(components) {
    if(!(components instanceof Array))
      components = [components];
    for(var i=components.length-1; i>=0; --i) {
      this.removeComponent(components[i]);
    }
  },
  removeComponent: function(component) {      
    ZOO.removeItem(this.components, component);
    // clearBounds() so that it gets recalculated on the next call
    // to this.getBounds();
    this.clearBounds();
  },
  getLength: function() {
    var length = 0.0;
    for (var i=0, len=this.components.length; i<len; i++) {
      length += this.components[i].getLength();
    }
    return length;
  },
  getArea: function() {
    var area = 0.0;
    for (var i=0, len=this.components.length; i<len; i++) {
      area += this.components[i].getArea();
    }
    return area;
  },
  getCentroid: function() {
    return this.components.length && this.components[0].getCentroid();
  },
  move: function(x, y) {
    for(var i=0, len=this.components.length; i<len; i++) {
      this.components[i].move(x, y);
    }
  },
  rotate: function(angle, origin) {
    for(var i=0, len=this.components.length; i<len; ++i) {
      this.components[i].rotate(angle, origin);
    }
  },
  resize: function(scale, origin, ratio) {
    for(var i=0; i<this.components.length; ++i) {
      this.components[i].resize(scale, origin, ratio);
    }
    return this;
  },
  distanceTo: function(geometry, options) {
    var edge = !(options && options.edge === false);
    var details = edge && options && options.details;
    var result, best;
    var min = Number.POSITIVE_INFINITY;
    for(var i=0, len=this.components.length; i<len; ++i) {
      result = this.components[i].distanceTo(geometry, options);
      distance = details ? result.distance : result;
      if(distance < min) {
        min = distance;
        best = result;
        if(min == 0)
          break;
      }
    }
    return best;
  },
  equals: function(geometry) {
    var equivalent = true;
    if(!geometry || !geometry.CLASS_NAME ||
       (this.CLASS_NAME != geometry.CLASS_NAME))
      equivalent = false;
    else if(!(geometry.components instanceof Array) ||
             (geometry.components.length != this.components.length))
      equivalent = false;
    else
      for(var i=0, len=this.components.length; i<len; ++i) {
        if(!this.components[i].equals(geometry.components[i])) {
          equivalent = false;
          break;
        }
      }
    return equivalent;
  },
  transform: function(source, dest) {
    if (source && dest) {
      for (var i=0, len=this.components.length; i<len; i++) {  
        var component = this.components[i];
        component.transform(source, dest);
      }
      this.bounds = null;
    }
    return this;
  },
  intersects: function(geometry) {
    var intersect = false;
    for(var i=0, len=this.components.length; i<len; ++ i) {
      intersect = geometry.intersects(this.components[i]);
      if(intersect)
        break;
    }
    return intersect;
  },
  getVertices: function(nodes) {
    var vertices = [];
    for(var i=0, len=this.components.length; i<len; ++i) {
      Array.prototype.push.apply(
          vertices, this.components[i].getVertices(nodes)
          );
    }
    return vertices;
  },
  CLASS_NAME: 'ZOO.Geometry.Collection'
});
ZOO.Geometry.Point = ZOO.Class(ZOO.Geometry, {
  x: null,
  y: null,
  initialize: function(x, y) {
    ZOO.Geometry.prototype.initialize.apply(this, arguments);
    this.x = parseFloat(x);
    this.y = parseFloat(y);
  },
  clone: function(obj) {
    if (obj == null)
      obj = new ZOO.Geometry.Point(this.x, this.y);
    // catch any randomly tagged-on properties
    //OpenLayers.Util.applyDefaults(obj, this);
    return obj;
  },
  calculateBounds: function () {
    this.bounds = new ZOO.Bounds(this.x, this.y,
                                        this.x, this.y);
  },
  distanceTo: function(geometry, options) {
    var edge = !(options && options.edge === false);
    var details = edge && options && options.details;
    var distance, x0, y0, x1, y1, result;
    if(geometry instanceof ZOO.Geometry.Point) {
      x0 = this.x;
      y0 = this.y;
      x1 = geometry.x;
      y1 = geometry.y;
      distance = Math.sqrt(Math.pow(x0 - x1, 2) + Math.pow(y0 - y1, 2));
      result = !details ?
        distance : {x0: x0, y0: y0, x1: x1, y1: y1, distance: distance};
    } else {
      result = geometry.distanceTo(this, options);
      if(details) {
        // switch coord order since this geom is target
        result = {
          x0: result.x1, y0: result.y1,
          x1: result.x0, y1: result.y0,
          distance: result.distance
        };
      }
    }
    return result;
  },
  equals: function(geom) {
    var equals = false;
    if (geom != null)
      equals = ((this.x == geom.x && this.y == geom.y) ||
                (isNaN(this.x) && isNaN(this.y) && isNaN(geom.x) && isNaN(geom.y)));
    return equals;
  },
  toShortString: function() {
    return (this.x + ", " + this.y);
  },
  move: function(x, y) {
    this.x = this.x + x;
    this.y = this.y + y;
    this.clearBounds();
  },
  rotate: function(angle, origin) {
        angle *= Math.PI / 180;
        var radius = this.distanceTo(origin);
        var theta = angle + Math.atan2(this.y - origin.y, this.x - origin.x);
        this.x = origin.x + (radius * Math.cos(theta));
        this.y = origin.y + (radius * Math.sin(theta));
        this.clearBounds();
  },
  getCentroid: function() {
    return new ZOO.Geometry.Point(this.x, this.y);
  },
  resize: function(scale, origin, ratio) {
    ratio = (ratio == undefined) ? 1 : ratio;
    this.x = origin.x + (scale * ratio * (this.x - origin.x));
    this.y = origin.y + (scale * (this.y - origin.y));
    this.clearBounds();
    return this;
  },
  intersects: function(geometry) {
    var intersect = false;
    if(geometry.CLASS_NAME == "ZOO.Geometry.Point") {
      intersect = this.equals(geometry);
    } else {
      intersect = geometry.intersects(this);
    }
    return intersect;
  },
  transform: function(source, dest) {
    if ((source && dest)) {
      ZOO.Projection.transform(
          this, source, dest); 
      this.bounds = null;
    }       
    return this;
  },
  getVertices: function(nodes) {
    return [this];
  },
  CLASS_NAME: 'ZOO.Geometry.Point'
});
ZOO.Geometry.Surface = ZOO.Class(ZOO.Geometry, {
  initialize: function() {
    ZOO.Geometry.prototype.initialize.apply(this, arguments);
  },
  CLASS_NAME: "ZOO.Geometry.Surface"
});
ZOO.Geometry.MultiPoint = ZOO.Class(
  ZOO.Geometry.Collection, {
  componentTypes: ["ZOO.Geometry.Point"],
  initialize: function(components) {
    ZOO.Geometry.Collection.prototype.initialize.apply(this,arguments);
  },
  addPoint: function(point, index) {
    this.addComponent(point, index);
  },
  removePoint: function(point){
    this.removeComponent(point);
  },
  CLASS_NAME: "ZOO.Geometry.MultiPoint"
});
ZOO.Geometry.Curve = ZOO.Class(ZOO.Geometry.MultiPoint, {
  componentTypes: ["ZOO.Geometry.Point"],
  initialize: function(points) {
    ZOO.Geometry.MultiPoint.prototype.initialize.apply(this,arguments);
  },
  getLength: function() {
    var length = 0.0;
    if ( this.components && (this.components.length > 1)) {
      for(var i=1, len=this.components.length; i<len; i++) {
        length += this.components[i-1].distanceTo(this.components[i]);
      }
    }
    return length;
  },
  CLASS_NAME: "ZOO.Geometry.Curve"
});
ZOO.Geometry.LineString = ZOO.Class(ZOO.Geometry.Curve, {
  initialize: function(points) {
    ZOO.Geometry.Curve.prototype.initialize.apply(this, arguments);        
  },
  removeComponent: function(point) {
    if ( this.components && (this.components.length > 2))
      ZOO.Geometry.Collection.prototype.removeComponent.apply(this,arguments);
  },
  intersects: function(geometry) {
    var intersect = false;
    var type = geometry.CLASS_NAME;
    if(type == "ZOO.Geometry.LineString" ||
       type == "ZOO.Geometry.LinearRing" ||
       type == "ZOO.Geometry.Point") {
      var segs1 = this.getSortedSegments();
      var segs2;
      if(type == "ZOO.Geometry.Point")
        segs2 = [{
          x1: geometry.x, y1: geometry.y,
          x2: geometry.x, y2: geometry.y
        }];
      else
        segs2 = geometry.getSortedSegments();
      var seg1, seg1x1, seg1x2, seg1y1, seg1y2,
          seg2, seg2y1, seg2y2;
      // sweep right
      outer: for(var i=0, len=segs1.length; i<len; ++i) {
         seg1 = segs1[i];
         seg1x1 = seg1.x1;
         seg1x2 = seg1.x2;
         seg1y1 = seg1.y1;
         seg1y2 = seg1.y2;
         inner: for(var j=0, jlen=segs2.length; j<jlen; ++j) {
           seg2 = segs2[j];
           if(seg2.x1 > seg1x2)
             break;
           if(seg2.x2 < seg1x1)
             continue;
           seg2y1 = seg2.y1;
           seg2y2 = seg2.y2;
           if(Math.min(seg2y1, seg2y2) > Math.max(seg1y1, seg1y2))
             continue;
           if(Math.max(seg2y1, seg2y2) < Math.min(seg1y1, seg1y2))
             continue;
           if(ZOO.Geometry.segmentsIntersect(seg1, seg2)) {
             intersect = true;
             break outer;
           }
         }
      }
    } else {
      intersect = geometry.intersects(this);
    }
    return intersect;
  },
  getSortedSegments: function() {
    var numSeg = this.components.length - 1;
    var segments = new Array(numSeg);
    for(var i=0; i<numSeg; ++i) {
      point1 = this.components[i];
      point2 = this.components[i + 1];
      if(point1.x < point2.x)
        segments[i] = {
          x1: point1.x,
          y1: point1.y,
          x2: point2.x,
          y2: point2.y
        };
      else
        segments[i] = {
          x1: point2.x,
          y1: point2.y,
          x2: point1.x,
          y2: point1.y
        };
    }
    // more efficient to define this somewhere static
    function byX1(seg1, seg2) {
      return seg1.x1 - seg2.x1;
    }
    return segments.sort(byX1);
  },
  splitWithSegment: function(seg, options) {
    var edge = !(options && options.edge === false);
    var tolerance = options && options.tolerance;
    var lines = [];
    var verts = this.getVertices();
    var points = [];
    var intersections = [];
    var split = false;
    var vert1, vert2, point;
    var node, vertex, target;
    var interOptions = {point: true, tolerance: tolerance};
    var result = null;
    for(var i=0, stop=verts.length-2; i<=stop; ++i) {
      vert1 = verts[i];
      points.push(vert1.clone());
      vert2 = verts[i+1];
      target = {x1: vert1.x, y1: vert1.y, x2: vert2.x, y2: vert2.y};
      point = ZOO.Geometry.segmentsIntersect(seg, target, interOptions);
      if(point instanceof ZOO.Geometry.Point) {
        if((point.x === seg.x1 && point.y === seg.y1) ||
           (point.x === seg.x2 && point.y === seg.y2) ||
            point.equals(vert1) || point.equals(vert2))
          vertex = true;
        else
          vertex = false;
        if(vertex || edge) {
          // push intersections different than the previous
          if(!point.equals(intersections[intersections.length-1]))
            intersections.push(point.clone());
          if(i === 0) {
            if(point.equals(vert1))
              continue;
          }
          if(point.equals(vert2))
            continue;
          split = true;
          if(!point.equals(vert1))
            points.push(point);
          lines.push(new ZOO.Geometry.LineString(points));
          points = [point.clone()];
        }
      }
    }
    if(split) {
      points.push(vert2.clone());
      lines.push(new ZOO.Geometry.LineString(points));
    }
    if(intersections.length > 0) {
      // sort intersections along segment
      var xDir = seg.x1 < seg.x2 ? 1 : -1;
      var yDir = seg.y1 < seg.y2 ? 1 : -1;
      result = {
        lines: lines,
        points: intersections.sort(function(p1, p2) {
           return (xDir * p1.x - xDir * p2.x) || (yDir * p1.y - yDir * p2.y);
        })
      };
    }
    return result;
  },
  split: function(target, options) {
    var results = null;
    var mutual = options && options.mutual;
    var sourceSplit, targetSplit, sourceParts, targetParts;
    if(target instanceof ZOO.Geometry.LineString) {
      var verts = this.getVertices();
      var vert1, vert2, seg, splits, lines, point;
      var points = [];
      sourceParts = [];
      for(var i=0, stop=verts.length-2; i<=stop; ++i) {
        vert1 = verts[i];
        vert2 = verts[i+1];
        seg = {
          x1: vert1.x, y1: vert1.y,
          x2: vert2.x, y2: vert2.y
        };
        targetParts = targetParts || [target];
        if(mutual)
          points.push(vert1.clone());
        for(var j=0; j<targetParts.length; ++j) {
          splits = targetParts[j].splitWithSegment(seg, options);
          if(splits) {
            // splice in new features
            lines = splits.lines;
            if(lines.length > 0) {
              lines.unshift(j, 1);
              Array.prototype.splice.apply(targetParts, lines);
              j += lines.length - 2;
            }
            if(mutual) {
              for(var k=0, len=splits.points.length; k<len; ++k) {
                point = splits.points[k];
                if(!point.equals(vert1)) {
                  points.push(point);
                  sourceParts.push(new ZOO.Geometry.LineString(points));
                  if(point.equals(vert2))
                    points = [];
                  else
                    points = [point.clone()];
                }
              }
            }
          }
        }
      }
      if(mutual && sourceParts.length > 0 && points.length > 0) {
        points.push(vert2.clone());
        sourceParts.push(new ZOO.Geometry.LineString(points));
      }
    } else {
      results = target.splitWith(this, options);
    }
    if(targetParts && targetParts.length > 1)
      targetSplit = true;
    else
      targetParts = [];
    if(sourceParts && sourceParts.length > 1)
      sourceSplit = true;
    else
      sourceParts = [];
    if(targetSplit || sourceSplit) {
      if(mutual)
        results = [sourceParts, targetParts];
      else
        results = targetParts;
    }
    return results;
  },
  splitWith: function(geometry, options) {
    return geometry.split(this, options);
  },
  getVertices: function(nodes) {
    var vertices;
    if(nodes === true)
      vertices = [
        this.components[0],
        this.components[this.components.length-1]
      ];
    else if (nodes === false)
      vertices = this.components.slice(1, this.components.length-1);
    else
      vertices = this.components.slice();
    return vertices;
  },
  distanceTo: function(geometry, options) {
    var edge = !(options && options.edge === false);
    var details = edge && options && options.details;
    var result, best = {};
    var min = Number.POSITIVE_INFINITY;
    if(geometry instanceof ZOO.Geometry.Point) {
      var segs = this.getSortedSegments();
      var x = geometry.x;
      var y = geometry.y;
      var seg;
      for(var i=0, len=segs.length; i<len; ++i) {
        seg = segs[i];
        result = ZOO.Geometry.distanceToSegment(geometry, seg);
        if(result.distance < min) {
          min = result.distance;
          best = result;
          if(min === 0)
            break;
        } else {
          // if distance increases and we cross y0 to the right of x0, no need to keep looking.
          if(seg.x2 > x && ((y > seg.y1 && y < seg.y2) || (y < seg.y1 && y > seg.y2)))
            break;
        }
      }
      if(details)
        best = {
          distance: best.distance,
          x0: best.x, y0: best.y,
          x1: x, y1: y
        };
      else
        best = best.distance;
    } else if(geometry instanceof ZOO.Geometry.LineString) { 
      var segs0 = this.getSortedSegments();
      var segs1 = geometry.getSortedSegments();
      var seg0, seg1, intersection, x0, y0;
      var len1 = segs1.length;
      var interOptions = {point: true};
      outer: for(var i=0, len=segs0.length; i<len; ++i) {
        seg0 = segs0[i];
        x0 = seg0.x1;
        y0 = seg0.y1;
        for(var j=0; j<len1; ++j) {
          seg1 = segs1[j];
          intersection = ZOO.Geometry.segmentsIntersect(seg0, seg1, interOptions);
          if(intersection) {
            min = 0;
            best = {
              distance: 0,
              x0: intersection.x, y0: intersection.y,
              x1: intersection.x, y1: intersection.y
            };
            break outer;
          } else {
            result = ZOO.Geometry.distanceToSegment({x: x0, y: y0}, seg1);
            if(result.distance < min) {
              min = result.distance;
              best = {
                distance: min,
                x0: x0, y0: y0,
                x1: result.x, y1: result.y
              };
            }
          }
        }
      }
      if(!details)
        best = best.distance;
      if(min !== 0) {
        // check the final vertex in this line's sorted segments
        if(seg0) {
          result = geometry.distanceTo(
              new ZOO.Geometry.Point(seg0.x2, seg0.y2),
              options
              );
          var dist = details ? result.distance : result;
          if(dist < min) {
            if(details)
              best = {
                distance: min,
                x0: result.x1, y0: result.y1,
                x1: result.x0, y1: result.y0
              };
            else
              best = dist;
          }
        }
      }
    } else {
      best = geometry.distanceTo(this, options);
      // swap since target comes from this line
      if(details)
        best = {
          distance: best.distance,
          x0: best.x1, y0: best.y1,
          x1: best.x0, y1: best.y0
        };
    }
    return best;
  },
  CLASS_NAME: "ZOO.Geometry.LineString"
});
ZOO.Geometry.LinearRing = ZOO.Class(
  ZOO.Geometry.LineString, {
  componentTypes: ["ZOO.Geometry.Point"],
  initialize: function(points) {
    ZOO.Geometry.LineString.prototype.initialize.apply(this,arguments);
  },
  addComponent: function(point, index) {
    var added = false;
    //remove last point
    var lastPoint = this.components.pop();
    // given an index, add the point
    // without an index only add non-duplicate points
    if(index != null || !point.equals(lastPoint))
      added = ZOO.Geometry.Collection.prototype.addComponent.apply(this,arguments);
    //append copy of first point
    var firstPoint = this.components[0];
    ZOO.Geometry.Collection.prototype.addComponent.apply(this,[firstPoint]);
    return added;
  },
  removeComponent: function(point) {
    if (this.components.length > 4) {
      //remove last point
      this.components.pop();
      //remove our point
      ZOO.Geometry.Collection.prototype.removeComponent.apply(this,arguments);
      //append copy of first point
      var firstPoint = this.components[0];
      ZOO.Geometry.Collection.prototype.addComponent.apply(this,[firstPoint]);
    }
  },
  move: function(x, y) {
    for(var i = 0, len=this.components.length; i<len - 1; i++) {
      this.components[i].move(x, y);
    }
  },
  rotate: function(angle, origin) {
    for(var i=0, len=this.components.length; i<len - 1; ++i) {
      this.components[i].rotate(angle, origin);
    }
  },
  resize: function(scale, origin, ratio) {
    for(var i=0, len=this.components.length; i<len - 1; ++i) {
      this.components[i].resize(scale, origin, ratio);
    }
    return this;
  },
  transform: function(source, dest) {
    if (source && dest) {
      for (var i=0, len=this.components.length; i<len - 1; i++) {
        var component = this.components[i];
        component.transform(source, dest);
      }
      this.bounds = null;
    }
    return this;
  },
  getCentroid: function() {
    if ( this.components && (this.components.length > 2)) {
      var sumX = 0.0;
      var sumY = 0.0;
      for (var i = 0; i < this.components.length - 1; i++) {
        var b = this.components[i];
        var c = this.components[i+1];
        sumX += (b.x + c.x) * (b.x * c.y - c.x * b.y);
        sumY += (b.y + c.y) * (b.x * c.y - c.x * b.y);
      }
      var area = -1 * this.getArea();
      var x = sumX / (6 * area);
      var y = sumY / (6 * area);
    }
    return new ZOO.Geometry.Point(x, y);
  },
  getArea: function() {
    var area = 0.0;
    if ( this.components && (this.components.length > 2)) {
      var sum = 0.0;
      for (var i=0, len=this.components.length; i<len - 1; i++) {
        var b = this.components[i];
        var c = this.components[i+1];
        sum += (b.x + c.x) * (c.y - b.y);
      }
      area = - sum / 2.0;
    }
    return area;
  },
  containsPoint: function(point) {
    var approx = OpenLayers.Number.limitSigDigs;
    var digs = 14;
    var px = approx(point.x, digs);
    var py = approx(point.y, digs);
    function getX(y, x1, y1, x2, y2) {
      return (((x1 - x2) * y) + ((x2 * y1) - (x1 * y2))) / (y1 - y2);
    }
    var numSeg = this.components.length - 1;
    var start, end, x1, y1, x2, y2, cx, cy;
    var crosses = 0;
    for(var i=0; i<numSeg; ++i) {
      start = this.components[i];
      x1 = approx(start.x, digs);
      y1 = approx(start.y, digs);
      end = this.components[i + 1];
      x2 = approx(end.x, digs);
      y2 = approx(end.y, digs);

      /**
       * The following conditions enforce five edge-crossing rules:
       *    1. points coincident with edges are considered contained;
       *    2. an upward edge includes its starting endpoint, and
       *    excludes its final endpoint;
       *    3. a downward edge excludes its starting endpoint, and
       *    includes its final endpoint;
       *    4. horizontal edges are excluded; and
       *    5. the edge-ray intersection point must be strictly right
       *    of the point P.
       */
      if(y1 == y2) {
        // horizontal edge
        if(py == y1) {
          // point on horizontal line
          if(x1 <= x2 && (px >= x1 && px <= x2) || // right or vert
              x1 >= x2 && (px <= x1 && px >= x2)) { // left or vert
            // point on edge
            crosses = -1;
            break;
          }
        }
        // ignore other horizontal edges
        continue;
      }
      cx = approx(getX(py, x1, y1, x2, y2), digs);
      if(cx == px) {
        // point on line
        if(y1 < y2 && (py >= y1 && py <= y2) || // upward
            y1 > y2 && (py <= y1 && py >= y2)) { // downward
          // point on edge
          crosses = -1;
          break;
        }
      }
      if(cx <= px) {
        // no crossing to the right
        continue;
      }
      if(x1 != x2 && (cx < Math.min(x1, x2) || cx > Math.max(x1, x2))) {
        // no crossing
        continue;
      }
      if(y1 < y2 && (py >= y1 && py < y2) || // upward
          y1 > y2 && (py < y1 && py >= y2)) { // downward
        ++crosses;
      }
    }
    var contained = (crosses == -1) ?
      // on edge
      1 :
      // even (out) or odd (in)
      !!(crosses & 1);

    return contained;
  },
  intersects: function(geometry) {
    var intersect = false;
    if(geometry.CLASS_NAME == "ZOO.Geometry.Point")
      intersect = this.containsPoint(geometry);
    else if(geometry.CLASS_NAME == "ZOO.Geometry.LineString")
      intersect = geometry.intersects(this);
    else if(geometry.CLASS_NAME == "ZOO.Geometry.LinearRing")
      intersect = ZOO.Geometry.LineString.prototype.intersects.apply(
          this, [geometry]
          );
    else
      for(var i=0, len=geometry.components.length; i<len; ++ i) {
        intersect = geometry.components[i].intersects(this);
        if(intersect)
          break;
      }
    return intersect;
  },
  getVertices: function(nodes) {
    return (nodes === true) ? [] : this.components.slice(0, this.components.length-1);
  },
  CLASS_NAME: "ZOO.Geometry.LinearRing"
});
ZOO.Geometry.MultiLineString = ZOO.Class(
  ZOO.Geometry.Collection, {
  componentTypes: ["ZOO.Geometry.LineString"],
  initialize: function(components) {
    ZOO.Geometry.Collection.prototype.initialize.apply(this,arguments);        
  },
  split: function(geometry, options) {
    var results = null;
    var mutual = options && options.mutual;
    var splits, sourceLine, sourceLines, sourceSplit, targetSplit;
    var sourceParts = [];
    var targetParts = [geometry];
    for(var i=0, len=this.components.length; i<len; ++i) {
      sourceLine = this.components[i];
      sourceSplit = false;
      for(var j=0; j < targetParts.length; ++j) { 
        splits = sourceLine.split(targetParts[j], options);
        if(splits) {
          if(mutual) {
            sourceLines = splits[0];
            for(var k=0, klen=sourceLines.length; k<klen; ++k) {
              if(k===0 && sourceParts.length)
                sourceParts[sourceParts.length-1].addComponent(
                  sourceLines[k]
                );
              else
                sourceParts.push(
                  new ZOO.Geometry.MultiLineString([
                    sourceLines[k]
                    ])
                );
            }
            sourceSplit = true;
            splits = splits[1];
          }
          if(splits.length) {
            // splice in new target parts
            splits.unshift(j, 1);
            Array.prototype.splice.apply(targetParts, splits);
            break;
          }
        }
      }
      if(!sourceSplit) {
        // source line was not hit
        if(sourceParts.length) {
          // add line to existing multi
          sourceParts[sourceParts.length-1].addComponent(
              sourceLine.clone()
              );
        } else {
          // create a fresh multi
          sourceParts = [
            new ZOO.Geometry.MultiLineString(
                sourceLine.clone()
                )
            ];
        }
      }
    }
    if(sourceParts && sourceParts.length > 1)
      sourceSplit = true;
    else
      sourceParts = [];
    if(targetParts && targetParts.length > 1)
      targetSplit = true;
    else
      targetParts = [];
    if(sourceSplit || targetSplit) {
      if(mutual)
        results = [sourceParts, targetParts];
      else
        results = targetParts;
    }
    return results;
  },
  splitWith: function(geometry, options) {
    var results = null;
    var mutual = options && options.mutual;
    var splits, targetLine, sourceLines, sourceSplit, targetSplit, sourceParts, targetParts;
    if(geometry instanceof ZOO.Geometry.LineString) {
      targetParts = [];
      sourceParts = [geometry];
      for(var i=0, len=this.components.length; i<len; ++i) {
        targetSplit = false;
        targetLine = this.components[i];
        for(var j=0; j<sourceParts.length; ++j) {
          splits = sourceParts[j].split(targetLine, options);
          if(splits) {
            if(mutual) {
              sourceLines = splits[0];
              if(sourceLines.length) {
                // splice in new source parts
                sourceLines.unshift(j, 1);
                Array.prototype.splice.apply(sourceParts, sourceLines);
                j += sourceLines.length - 2;
              }
              splits = splits[1];
              if(splits.length === 0) {
                splits = [targetLine.clone()];
              }
            }
            for(var k=0, klen=splits.length; k<klen; ++k) {
              if(k===0 && targetParts.length) {
                targetParts[targetParts.length-1].addComponent(
                    splits[k]
                    );
              } else {
                targetParts.push(
                    new ZOO.Geometry.MultiLineString([
                      splits[k]
                      ])
                    );
              }
            }
            targetSplit = true;                    
          }
        }
        if(!targetSplit) {
          // target component was not hit
          if(targetParts.length) {
            // add it to any existing multi-line
            targetParts[targetParts.length-1].addComponent(
                targetLine.clone()
                );
          } else {
            // or start with a fresh multi-line
            targetParts = [
              new ZOO.Geometry.MultiLineString([
                  targetLine.clone()
                  ])
              ];
          }

        }
      }
    } else {
      results = geometry.split(this);
    }
    if(sourceParts && sourceParts.length > 1)
      sourceSplit = true;
    else
      sourceParts = [];
    if(targetParts && targetParts.length > 1)
      targetSplit = true;
    else
      targetParts = [];
    if(sourceSplit || targetSplit) {
      if(mutual)
        results = [sourceParts, targetParts];
      else
        results = targetParts;
    }
    return results;
  },
  CLASS_NAME: "ZOO.Geometry.MultiLineString"
});
ZOO.Geometry.Polygon = ZOO.Class(
  ZOO.Geometry.Collection, {
  componentTypes: ["ZOO.Geometry.LinearRing"],
  initialize: function(components) {
    ZOO.Geometry.Collection.prototype.initialize.apply(this,arguments);
  },
  getArea: function() {
    var area = 0.0;
    if ( this.components && (this.components.length > 0)) {
      area += Math.abs(this.components[0].getArea());
      for (var i=1, len=this.components.length; i<len; i++) {
        area -= Math.abs(this.components[i].getArea());
      }
    }
    return area;
  },
  containsPoint: function(point) {
    var numRings = this.components.length;
    var contained = false;
    if(numRings > 0) {
    // check exterior ring - 1 means on edge, boolean otherwise
      contained = this.components[0].containsPoint(point);
      if(contained !== 1) {
        if(contained && numRings > 1) {
          // check interior rings
          var hole;
          for(var i=1; i<numRings; ++i) {
            hole = this.components[i].containsPoint(point);
            if(hole) {
              if(hole === 1)
                contained = 1;
              else
                contained = false;
              break;
            }
          }
        }
      }
    }
    return contained;
  },
  intersects: function(geometry) {
    var intersect = false;
    var i, len;
    if(geometry.CLASS_NAME == "ZOO.Geometry.Point") {
      intersect = this.containsPoint(geometry);
    } else if(geometry.CLASS_NAME == "ZOO.Geometry.LineString" ||
              geometry.CLASS_NAME == "ZOO.Geometry.LinearRing") {
      // check if rings/linestrings intersect
      for(i=0, len=this.components.length; i<len; ++i) {
        intersect = geometry.intersects(this.components[i]);
        if(intersect) {
          break;
        }
      }
      if(!intersect) {
        // check if this poly contains points of the ring/linestring
        for(i=0, len=geometry.components.length; i<len; ++i) {
          intersect = this.containsPoint(geometry.components[i]);
          if(intersect) {
            break;
          }
        }
      }
    } else {
      for(i=0, len=geometry.components.length; i<len; ++ i) {
        intersect = this.intersects(geometry.components[i]);
        if(intersect)
          break;
      }
    }
    // check case where this poly is wholly contained by another
    if(!intersect && geometry.CLASS_NAME == "ZOO.Geometry.Polygon") {
      // exterior ring points will be contained in the other geometry
      var ring = this.components[0];
      for(i=0, len=ring.components.length; i<len; ++i) {
        intersect = geometry.containsPoint(ring.components[i]);
        if(intersect)
          break;
      }
    }
    return intersect;
  },
  distanceTo: function(geometry, options) {
    var edge = !(options && options.edge === false);
    var result;
    // this is the case where we might not be looking for distance to edge
    if(!edge && this.intersects(geometry))
      result = 0;
    else
      result = ZOO.Geometry.Collection.prototype.distanceTo.apply(
          this, [geometry, options]
          );
    return result;
  },
  CLASS_NAME: "ZOO.Geometry.Polygon"
});
ZOO.Geometry.MultiPolygon = ZOO.Class(
  ZOO.Geometry.Collection, {
  componentTypes: ["ZOO.Geometry.Polygon"],
  initialize: function(components) {
    ZOO.Geometry.Collection.prototype.initialize.apply(this,arguments);
  },
  CLASS_NAME: "ZOO.Geometry.MultiPolygon"
});

ZOO.Process = ZOO.Class({
  schemaLocation: "http://www.opengis.net/wps/1.0.0/../wpsExecute_request.xsd",
  namespaces: {
    ows: "http://www.opengis.net/ows/1.1",
    wps: "http://www.opengis.net/wps/1.0.0",
    xlink: "http://www.w3.org/1999/xlink",
    xsi: "http://www.w3.org/2001/XMLSchema-instance",
  },
  url: 'http://localhost/zoo',
  identifier: null,
  initialize: function(url,identifier) {
    this.url = url;
    this.identifier = identifier;
  },
  Execute: function(inputs) {
    if (this.identifier == null)
      return null;
    var body = new XML('<wps:Execute service="WPS" version="1.0.0" xmlns:wps="'+this.namespaces['wps']+'" xmlns:ows="'+this.namespaces['ows']+'" xmlns:xlink="'+this.namespaces['xlink']+'" xmlns:xsi="'+this.namespaces['xsi']+'" xsi:schemaLocation="'+this.schemaLocation+'"><ows:Identifier>'+this.identifier+'</ows:Identifier>'+this.buildDataInputsNode(inputs)+'</wps:Execute>');
    body = body.toXMLString();
    var response = ZOO.Request.Post(this.url,body,['Content-Type: text/xml; charset=UTF-8']);
    return response;
  },
  buildInput: {
    'complex': function(identifier,data) {
      var input = new XML('<wps:Input xmlns:wps="'+this.namespaces['wps']+'"><ows:Identifier xmlns:ows="'+this.namespaces['ows']+'">'+identifier+'</ows:Identifier><wps:Data><wps:ComplexData>'+data.value+'</wps:ComplexData></wps:Data></wps:Input>');
      input.*::Data.*::ComplexData.@mimeType = data.mimetype ? data.mimetype : 'text/plain';
      if (data.encoding)
        input.*::Data.*::ComplexData.@encoding = data.encoding;
      if (data.schema)
        input.*::Data.*::ComplexData.@schema = data.schema;
      input = input.toXMLString();
      return input;
    },
    'reference': function(identifier,data) {
      return '<wps:Input xmlns:wps="'+this.namespaces['wps']+'"><ows:Identifier xmlns:ows="'+this.namespaces['ows']+'">'+identifier+'</ows:Identifier><wps:Reference xmlns:xlink="'+this.namespaces['xlink']+'" xlink:href="'+data.value.replace('&','&amp;','gi')+'"/></wps:Input>';
    },
    'literal': function(identifier,data) {
      var input = new XML('<wps:Input xmlns:wps="'+this.namespaces['wps']+'"><ows:Identifier xmlns:ows="'+this.namespaces['ows']+'">'+identifier+'</ows:Identifier><wps:Data><wps:LiteralData>'+data.value+'</wps:LiteralData></wps:Data></wps:Input>');
      if (data.type)
        input.*::Data.*::LiteralData.@dataType = data.type;
      if (data.uom)
        input.*::Data.*::LiteralData.@uom = data.uom;
      input = input.toXMLString();
      return input;
    }
  },
  buildDataInputsNode:function(inputs){
    var data, builder, inputsArray=[];
    for (var attr in inputs) {
      data = inputs[attr];
      if (data.mimetype || data.type == 'complex')
        builder = this.buildInput['complex'];
      else if (data.type == 'reference' || data.type == 'url')
        builder = this.buildInput['reference'];
      else
        builder = this.buildInput['literal'];
      inputsArray.push(builder.apply(this,[attr,data]));
    }
    return '<wps:DataInputs xmlns:wps="'+this.namespaces['wps']+'">'+inputsArray.join('\n')+'</wps:DataInputs>';
  },
  CLASS_NAME: "ZOO.Process"
});
