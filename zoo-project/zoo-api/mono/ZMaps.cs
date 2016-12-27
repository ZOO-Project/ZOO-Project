/*
 * Author : Gérald FENOY
 *
 * Copyright (c) 2016 GeoLabs SARL
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
using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Runtime.CompilerServices;

public static class ZOO_API{
    public static int SERVICE_SUCCEEDED=3;
    public static int SERVICE_FAILED=4;
    [MethodImplAttribute(MethodImplOptions.InternalCall)]
    extern public static string Translate(string str);
    [MethodImplAttribute(MethodImplOptions.InternalCall)]
    extern public static void UpdateStatus([In] ZooGenerics.ZMaps obj,string str,int p);
};

namespace ZooGenerics
{
    public class KeysList : List<String> { };
    public class ZMap : Dictionary<String,String> {
	public int getKeysCount(){
	    List<String> tmp=new List<String>(this.Keys);
	    return tmp.Count;
	}
	public String getKey(int i){
	    List<String> tmp=new List<String>(this.Keys);
	    return tmp[i];
	}
	public void addToMap(String name,String value){
	    this.Add(name,value);
	}
	public String getMap(String name){
	    String test;
	    if(this.TryGetValue(name, out test)){
		return test;
	    }
	    return null;
	}
	public byte[] getMapAsBytes(String name){
	    String test;
	    if(this.TryGetValue(name, out test)){
		return System.Text.Encoding.UTF8.GetBytes(test);
	    }
	    return null;
	}
	public int getSize(){
	    String test;
	    if(this.TryGetValue("size", out test)){
		return int.Parse(test);
	    }
	    return -1;
	}
    };
    public class _ZMaps
    {
	private ZMap content; 
	private Dictionary<string,_ZMaps> child;
	public _ZMaps(){
	    this.content=null;
	    this.child=null;
	}
	public void setContent(ZMap content){
	    this.content=content;
	}
	public void setChild(Dictionary<string,_ZMaps> content){
	    this.child=content;
	}
	public void AddContent(String key,String value){
	    this.content.Add(key,value);
	}
	public ZMap getContent(){
	    return this.content;
	}
	public Dictionary<string,_ZMaps> getChild(){
	    return this.child;
	}
    }
    public class ZMaps : Dictionary<String,_ZMaps> {
	public int getKeysCount(){
	    List<String> tmp=new List<String>(this.Keys);
	    return tmp.Count;
	}
	public String getKey(int i){
	    List<String> tmp=new List<String>(this.Keys);
	    return tmp[i];
	}
	public _ZMaps getValue(String key){
	    return this[key];
	}
	public List<String> getKeys(){
	    return new List<String>(this.Keys);
	}
	public void addToMaps(String name,_ZMaps content){
	    this.Add(name,content);
	}
	public void setMapsInMaps(String mname,String name,String value){
	    _ZMaps test;
	    if (this.TryGetValue(mname, out test)) // Returns true.
	    {
		ZMap content=test.getContent();
		content.addToMap(name,value);
	    }
	}
	public _ZMaps getMaps(String name){
	    _ZMaps test;
	    if(this.TryGetValue(name, out test)){
		return test;
	    }
	    return null;
	}
    };
}
