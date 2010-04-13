ccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc
c     Simply create a welcome message
ccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc
       Integer FUNCTION HELLOF(zoo_main_cfg,zoo_inputs,zoo_outputs)
     & RESULT (R)
       Integer R, ls, iLenStr
       CHARACTER*(1024) zoo_main_cfg(10,30),zoo_inputs(10,30),
     & zoo_outputs(10,30)
        CHARACTER*(1024) TMP

       write(0,*) 'Hello '//zoo_inputs(4,1)//' from the Fortran world !'

       ls = iLenStr(zoo_inputs(4,1))
       TMP = zoo_inputs(4,1)
       zoo_outputs(1,1) = 'name'//CHAR(0)
       zoo_outputs(2,1) = 'result'//CHAR(0)
       zoo_outputs(3,1) = 'value'//CHAR(0)
       zoo_outputs(4,1) = 'Hello '//TMP(1:ls)//
     & ' from the Fortran world !'//CHAR(0)
       zoo_outputs(5,1) = 'datatype'//CHAR(0)
       zoo_outputs(6,1) = 'string'//CHAR(0)

       R = 3
       Return
       END

       Integer Function iLenStr(cString)
ccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc
c     Compute String Length (thanks to Abdelatif Djerboua from RHEA™)
ccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc
       Character*(*) cString
       Integer       iLen,i

      iLen = Len(cString)
      Do i=iLen,1,-1
         If(ichar(cString(i:i)).NE.0) Goto 10
      EndDo
      i = 1
  10  Continue
      iLenStr = i

      Return
      End Function iLenStr

