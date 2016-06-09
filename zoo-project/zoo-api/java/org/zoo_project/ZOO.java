package org.zoo_project;

import java.lang.*;
import java.util.*;

public class ZOO {
    static { System.loadLibrary("ZOO"); }
    public static Integer SERVICE_SUCCEEDED=3;
    public static Integer SERVICE_FAILED=4;
    public static native String translate(String a);
    public static native Integer updateStatus(HashMap conf,String pourcent,String message);
}
