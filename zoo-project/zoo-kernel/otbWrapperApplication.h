/*
  * Copyright (C) 2005-2024 Centre National d'Etudes Spatiales (CNES)
  *
  * This file is part of Orfeo Toolbox
  *
  *     https://www.orfeo-toolbox.org/
  *
  * Licensed under the Apache License, Version 2.0 (the "License");
  * you may not use this file except in compliance with the License.
  * You may obtain a copy of the License at
  *
  *     http://www.apache.org/licenses/LICENSE-2.0
  *
  * Unless required by applicable law or agreed to in writing, software
  * distributed under the License is distributed on an "AS IS" BASIS,
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  */

 #ifndef otbWrapperApplication_h
 #define otbWrapperApplication_h

 #include <string>
 #include <set>
 #include "otbWrapperTypes.h"
 #include "otbWrapperTags.h"
 #include "otbWrapperParameterGroup.h"
 #include "otbWrapperFieldParameter.h"

 #include "otbLogger.h"
 #include "otbStopwatch.h"
 #include "otbWrapperMacros.h"
 #include "otbWrapperInputImageParameter.h"
 #include "otbWrapperInputImageListParameter.h"
 #include "otbWrapperOutputImageParameter.h"
 #include "otbWrapperDocExampleStructure.h"
 #include "itkMersenneTwisterRandomVariateGenerator.h"
 #include "OTBApplicationEngineExport.h"

 namespace otb
 {

 class OTBApplicationEngine_EXPORT ApplicationException : public itk::ExceptionObject
 {
 public:
   itkTypeMacro(ApplicationException, ExceptionObject);

   ApplicationException(const char* file, unsigned int line, const char* message = "Application error.", const char* loc = "Unknown");

   ApplicationException(const std::string& file, unsigned int line, const char* message = "Application error.", const char* loc = "Unknown");

   ApplicationException(const std::string& file, unsigned int line, const std::string& message = "Application error.", const std::string& loc = "Unknown");
 };

 namespace Wrapper
 {

 class OTBApplicationEngine_EXPORT Application : public itk::Object
 {
 public:
   typedef Application                   Self;
   typedef itk::Object                   Superclass;
   typedef itk::SmartPointer<Self>       Pointer;
   typedef itk::SmartPointer<const Self> ConstPointer;

   itkTypeMacro(Application, itk::Object);

   virtual void        SetName(const std::string&);
   virtual const char* GetName() const;

   void                SetDescription(const std::string&);
   virtual const char* GetDescription() const;

   void Init();

   bool IsInitialized() const;

   void LoadParametersFromXML(const std::string& filename);
   void SaveParametersToXML(const std::string& filename);

   void UpdateParameters();

   int Execute();

   void WriteOutput();

   int ExecuteAndWriteOutput();

   bool ConnectImage(std::string in, Application* app, std::string out);

   void PropagateConnectMode(bool isMem);

   void Stop();

   /* Get the internal application parameters
    *
    * WARNING: this method may disappear from the API */
   ParameterGroup* GetParameterList();

   /* Get the internal application parameter specified
    * if the follow flag is on, the function returns the target of proxy parameters
    * WARNING: this method may disappear from the API */
   Parameter* GetParameterByKey(std::string const& parameter, bool follow = true);

   /* Get the internal application parameter specified
    *
    * WARNING: this method may disappear from the API */
   const Parameter* GetParameterByKey(std::string const& parameter, bool follow = true) const;

   /* Returns the description of a parameter */
   std::string GetParameterName(std::string const& paramKey);

   /* Returns the description of a parameter */
   std::string GetParameterDescription(std::string const& paramKey);

   /* Set the description of a parameter */
   void SetParameterDescription(std::string const& paramKey, std::string dec);

   /* Enable the use of an optional parameter. Returns the previous state */
   void EnableParameter(std::string const& paramKey);

   /* Disable the use of an optional parameter. Returns the previous state  */
   void DisableParameter(std::string const& paramKey);

   /* Return the enable state of an optional parameter  */
   bool IsParameterEnabled(std::string const& paramKey, bool recurseParents = false) const;

   /* Return true if the specified parameter is mandatory */
   bool IsMandatory(std::string const& paramKey) const;

   /* Return true if the specified parameter was set automatically in
    * the application
    */
   bool HasAutomaticValue(std::string const& paramKey) const;

   /* Returns true if the parameter has an associated value provided externally
    *  (not automatically computed by the application) */
   bool HasUserValue(std::string const& paramKey) const;

   /* If a user value was provided, clear it and update the other parameters */
   void ClearValue(std::string const& paramKey);

   /* Returns true if the parameter has an associated value.
    * This value can be an automatically computed value or default value,
    * or a value set externally by user */
   bool HasValue(std::string const& paramKey) const;

   void SetParameterUserValue(std::string const& paramKey, bool value);

   /* Return the user level of access to a parameter */
   UserLevel GetParameterUserLevel(std::string const& paramKey) const;

   Role GetParameterRole(std::string const& paramKey) const;

   /* Get the parameter type from its name */
   ParameterType GetParameterType(std::string const& paramKey) const;

   /* Returns the description of a parameter */
   std::vector<std::string> GetChoiceKeys(std::string const& paramKey);

   /* Returns the description of a parameter */
   std::vector<std::string> GetChoiceNames(std::string const& paramKey);

   /* Set an integer value
    *
    * Can be called for types :
    * \li ParameterType_Int
    * \li ParameterType_Bool
    * \li ParameterType_Float
    * \li ParameterType_Double
    * \li ParameterType_Radius
    * \li ParameterType_Choice
    */
   void SetParameterInt(std::string const& parameter, int value, bool hasUserValueFlag = true);

   /* Set a floating value
    *
    * Can be called for types :
    * \li ParameterType_Float
    * \li ParameterType_Double
    */
   void SetParameterFloat(std::string const& parameter, float value, bool hasUserValueFlag = true);

   /* Set a double precision floating value
    *
    * Can be called for types :
    * \li ParameterType_Float
    * \li ParameterType_Double
    */
   void SetParameterDouble(std::string const& parameter, double value, bool hasUserValueFlag = true);

   /* Set a string value
    *
    * Can be called for types :
    * \li ParameterType_InputImageListParameter
    * \li ParameterType_InputVectorDataListParameter
    * \li ParameterType_InputFilenameListParameter
    * \li ParameterType_StringList
    * \li ParameterType_ListView
    * \li ParameterType_Band
    * \li ParameterType_Field
    */
   void SetParameterString(std::string const& parameter, std::string value, bool hasUserValueFlag = true);

   /* Set a string value
    *
    * Can be called for types :
    * \li ParameterType_String
    * \li ParameterType_StringList
    * \li ParameterType_ListView
    * \li ParameterType_Band
    * \li ParameterType_Field
    * \li ParameterType_InputFilename
    * \li ParameterType_OutputFilename
    * \li ParameterType_Directory
    * \li ParameterType_Choice
    * \li ParameterType_Float
    * \li ParameterType_Double
    * \li ParameterType_Int
    * \li ParameterType_Radius
    * \li ParameterType_InputImageParameter
    * \li ParameterType_InputVectorDataParameter
    * \li ParameterType_OutputImageParameter
    * \li ParameterType_OutputVectorDataParameter
    */
   void SetParameterStringList(std::string const& parameter, std::vector<std::string> values, bool hasUserValueFlag = true);

   bool IsApplicationReady();

   bool IsParameterMissing(const std::string& key) const;

   /* Set a default integer value, must be used in the
    * DoInit when setting a value by default
    * for the parameter
    *
    * Can be called for types :
    * \li ParameterType_Int
    * \li ParameterType_Float
    * \li ParameterType_Double
    * \li ParameterType_Radius
    * \li ParameterType_Choice
    */
   void SetDefaultParameterInt(std::string const& parameter, int value);

   /* Get the default integer value of a parameter
    *
    * Can be called for types :
    * \li ParameterType_Int
    * \li ParameterType_Float
    * \li ParameterType_Double
    * \li ParameterType_Radius
    * \li ParameterType_Choice
    */
   int GetDefaultParameterInt(std::string const& parameter);

   /* Set a default floating value, must be used in the
    * DoInit when setting a value by default
    * for the parameter
    *
    * Can be called for types :
    * \li ParameterType_Float
    * \li ParameterType_Double
    */
   void SetDefaultParameterFloat(std::string const& parameter, float value);

   /* Get the default floating value of a parameter
    *
    * Can be called for types :
    * \li ParameterType_Float
    * \li ParameterType_Double
    */
   float GetDefaultParameterFloat(std::string const& parameter);

   /* Set a default double precision floating value, must be used in the
    * DoInit when setting a value by default
    * for the parameter
    *
    * Can be called for types :
    * \li ParameterType_Float
    * \li ParameterType_Double
    */
   void SetDefaultParameterDouble(std::string const& parameter, double value);

   /* Get the default double precision floating value of a parameter
    *
    * Can be called for types :
    * \li ParameterType_Float
    * \li ParameterType_Double
    */
   double GetDefaultParameterDouble(std::string const& parameter);

   void SetDefaultOutputPixelType(std::string const& parameter, ImagePixelType type);

   void SetDefaultOutputComplexPixelType(std::string const& parameter, ComplexImagePixelType type);

   /* Set a minimum int value, must used in the
    * DoInit when setting a value by default
    * for the parameter
    *
    * Can be called for types :
    * \li ParameterType_Int
    */
   void SetMinimumParameterIntValue(std::string const& parameter, int value);

   /* Set a maximum int value, must used in the
    * DoInit when setting a value by default
    * for the parameter
    *
    * Can be called for types :
    * \li ParameterType_Int
    */
   void SetMaximumParameterIntValue(std::string const& parameter, int value);

   /* Set a minimum float value, must used in the
    * DoInit when setting a value by default
    * for the parameter
    *
    * Can be called for types :
    * \li ParameterType_Float
    */
   void SetMinimumParameterFloatValue(std::string const& parameter, float value);

   /* Set a maximum float value, must used in the
    * DoInit when setting a value by default
    * for the parameter
    *
    * Can be called for types :
    * \li ParameterType_Float
    */
   void SetMaximumParameterFloatValue(std::string const& parameter, float value);

   /* Set a minimum double precision float value, must used in the
    * DoInit when setting a value by default
    * for the parameter
    *
    * Can be called for types :
    * \li ParameterType_Float
    * \li ParameterType_Double
    */
   void SetMinimumParameterDoubleValue(std::string const& parameter, double value);

   /* Set a maximum double precision value, must used in the
    * DoInit when setting a value by default
    * for the parameter
    *
    * Can be called for types :
    * \li ParameterType_Float
    * \li ParameterType_Double
    */
   void SetMaximumParameterDoubleValue(std::string const& parameter, double value);


   void SetListViewSingleSelectionMode(std::string const& parameter, bool status);

   bool GetListViewSingleSelectionMode(const std::string& parameter);

   /* Set an output image value
    *
    * Can be called for types :
    * \li ParameterType_OutputImage
    */
   void SetParameterOutputImage(std::string const& parameter, FloatVectorImageType* value);

   /* Set the pixel type in which the image will be saved
    *
    * Can be called for types :
    * \li ParameterType_OutputImage
    */
   void SetParameterOutputImagePixelType(std::string const& parameter, ImagePixelType pixelType);

   /* Set an output vector data value
    *
    * Can be called for types :
    * \li ParameterType_OutputVectorData
    */
   void SetParameterOutputVectorData(std::string const& parameter, VectorDataType* value);

   /* Get an integer parameter value
    *
    * Can be called for types :
    * \li ParameterType_Int
    * \li ParameterType_Bool
    * \li ParameterType_Float
    * \li ParameterType_Double
    * \li ParameterType_Radius
    * \li ParameterType_Choice
    */
   int GetParameterInt(std::string const& parameter) const;

   /* Get a floating parameter value
    *
    * Can be called for types :
    * \li ParameterType_Float
    * \li ParameterType_Double
    */
   float GetParameterFloat(std::string const& parameter) const;

   /* Get a double precision floating parameter value
    *
    * Can be called for types :
    * \li ParameterType_Float
    * \li ParameterType_Double
    */
   double GetParameterDouble(std::string const& parameter) const;

   /* Get a string parameter value
    *
    * Can be called for types :
    * \li ParameterType_String
    * \li ParameterType_InputFilename
    * \li ParameterType_OutputFilename
    * \li ParameterType_Directory
    * \li ParameterType_InputImage
    * \li ParameterType_InputVectorData
    * \li ParameterType_OutputImage
    * \li ParameterType_OutputVectorData
    */
   std::string GetParameterString(std::string const& parameter) const;

   /* Get a string list parameter value
    *
    * Can be called for types :
    * \li ParameterType_StringList
    * \li ParameterType_InputImageList
    * \li ParameterType_InputFilenameList
    */
   // TODO: Should be rewritten:
   //
   // std::size_t
   // GetParameterStringList( const std::vector< String > & v,
   //                         const std::string & parameter ) const;
   //
   // to avoid useless memory allocations.
   std::vector<std::string> GetParameterStringList(const std::string& parameter);


   void SetParameterInputImage(std::string const& parameter, ImageBaseType* inputImage);

   ImageBaseType* GetParameterOutputImage(std::string const& parameter);

   void AddImageToParameterInputImageList(std::string const& parameter, ImageBaseType* img);

   void SetNthParameterInputImageList(std::string const& parameter, const unsigned int& id, ImageBaseType* img);

   void SetVectorData(std::string const& key, std::string const& vectorData );

   void SetTypeFilter(std::string const& key, FieldParameter::TypeFilterType const& typeFilter );

   const FieldParameter::TypeFilterType& GetTypeFilter(std::string const& key ) const;

   void SetRasterData(std::string const& key, std::string const& rasterData );

   void AddParameterStringList(std::string const& parameter, const std::string& str);

   void SetNthParameterStringList(std::string const& parameter, const unsigned int& id, const std::string& str);


   void ClearParameterInputImageList(std::string const& parameter);

   unsigned int GetNumberOfElementsInParameterInputImageList(std::string const& parameter);


   /* Get an image value
    *
    * Can be called for types :
    * \li ParameterType_InputImage
    */
   FloatVectorImageType* GetParameterImage(std::string const& parameter);

   UInt8ImageType*        GetParameterUInt8Image(std::string const&);
   UInt16ImageType*       GetParameterUInt16Image(std::string const&);
   Int16ImageType*        GetParameterInt16Image(std::string const&);
   UInt32ImageType*       GetParameterUInt32Image(std::string const&);
   Int32ImageType*        GetParameterInt32Image(std::string const&);
   FloatImageType*        GetParameterFloatImage(std::string const&);
   DoubleImageType*       GetParameterDoubleImage(std::string const&);
   UInt8VectorImageType*  GetParameterUInt8VectorImage(std::string const&);
   UInt16VectorImageType* GetParameterUInt16VectorImage(std::string const&);
   Int16VectorImageType*  GetParameterInt16VectorImage(std::string const&);
   UInt32VectorImageType* GetParameterUInt32VectorImage(std::string const&);
   Int32VectorImageType*  GetParameterInt32VectorImage(std::string const&);
   FloatVectorImageType*  GetParameterFloatVectorImage(std::string const&);
   DoubleVectorImageType* GetParameterDoubleVectorImage(std::string const&);
   UInt8RGBImageType*     GetParameterUInt8RGBImage(std::string const&);
   UInt8RGBAImageType*    GetParameterUInt8RGBAImage(std::string const&);

   // Complex image
   ComplexInt16ImageType*  GetParameterComplexInt16Image(std::string const&);
   ComplexInt32ImageType*  GetParameterComplexInt32Image(std::string const&);
   ComplexFloatImageType*  GetParameterComplexFloatImage(std::string const&);
   ComplexDoubleImageType* GetParameterComplexDoubleImage(std::string const&);

   ComplexInt16VectorImageType*  GetParameterComplexInt16VectorImage(std::string const&);
   ComplexInt32VectorImageType*  GetParameterComplexInt32VectorImage(std::string const&);
   ComplexFloatVectorImageType*  GetParameterComplexFloatVectorImage(std::string const&);
   ComplexDoubleVectorImageType* GetParameterComplexDoubleVectorImage(std::string const&);


   /* Get an image list value
    *
    * Can be called for types :
    * \li ParameterType_InputImageList
    */
   FloatVectorImageListType* GetParameterImageList(std::string const& parameter);

   /* GetParameterVectorData
    *
    * Can be called for types :

    * \li ParameterType_InputVectorData
    */
   VectorDataType* GetParameterVectorData(std::string const& parameter);

   /* GetParameteVetorDataList
      *
      * Can be called for types :

      * \li ParameterType_InputVectorDatalist
      */
   VectorDataListType* GetParameterVectorDataList(std::string const& parameter);

   /* Get the parameter as a std::string
     *
     * Can be called for types :
     * \li ParameterType_Float
     * \li ParameterType_Double
     * \li ParameterType_Int
     * \li ParameterType_Choice
     * \li ParameterType_Radius
     * \li ParameterType_String
     * \li ParameterType_InputFilename
     * \li ParameterType_OutputFilename
     * \li ParameterType_Directory
     * \li ParameterType_InputImage
     * \li ParameterType_InputVectorData
     * \li ParameterType_OutputImage
     * \li ParameterType_OutputVectorData
     */
   std::string GetParameterAsString(std::string const& paramKey);

   /* Get the list of all parameters
    */
   std::vector<std::string> GetParametersKeys(bool recursive = true);

   /* Get the pixel type in which the image will be saved
    *
    * Can be called for types :
    * \li ParameterType_OutputImage
    */
   ImagePixelType GetParameterOutputImagePixelType(std::string const& parameter);

   void SetParameterList(ParameterGroup::Pointer paramGroup)
   {
     m_ParameterList = paramGroup;
   }

   otb::Logger* GetLogger() const;

   void SetLogger(otb::Logger* logger);

   itk::ProcessObject* GetProgressSource() const;

   std::string GetProgressDescription() const;

   virtual void        SetDocLongDescription(const std::string&);
   virtual const char* GetDocLongDescription() const;

   virtual void        SetDocAuthors(const std::string&);
   virtual const char* GetDocAuthors() const;

   virtual void        SetDocLimitations(const std::string&);
   virtual const char* GetDocLimitations() const;

   virtual void        SetDocSeeAlso(const std::string&);
   virtual const char* GetDocSeeAlso() const;

   virtual void                     SetDocTags(std::vector<std::string>);
   virtual std::vector<std::string> GetDocTags() const;

   void AddDocTag(const std::string&);

   bool IsDeprecated();

   DocExampleStructure::Pointer GetDocExample();
   unsigned int                 GetNumberOfExamples();
   std::string GetExampleComment(unsigned int id);
   unsigned int GetExampleNumberOfParameters(unsigned int id);
   std::string GetExampleParameterKey(unsigned int exId, unsigned int paramId);
   std::string GetExampleParameterValue(unsigned int exId, unsigned int paramId);
   void SetDocExampleParameterValue(const std::string key, const std::string value, unsigned int exId = 0);
   void SetExampleComment(const std::string& comm, unsigned int i);
   unsigned int AddExample(const std::string& comm = "");
   std::string GetCLExample();
   std::string GetHtmlExample();

   std::vector<std::pair<std::string, std::string>> GetOutputParametersSumUp();

   double GetLastExecutionTiming() const;

   virtual void SetDocLink(const std::string& link);
   virtual const std::string& GetDocLink() const;

   void SetOfficialDocLink();

   ImageBaseType::PointType GetImageOrigin(const std::string& key, unsigned int idx = 0);

   ImageBaseType::SpacingType GetImageSpacing(const std::string& key, unsigned int idx = 0);

   ImageBaseType::SizeType GetImageSize(const std::string& key, unsigned int idx = 0);

   unsigned int GetImageNbBands(const std::string& key, unsigned int idx = 0);

   std::string GetImageProjection(const std::string& key, unsigned int idx = 0);

   unsigned long PropagateRequestedRegion(const std::string& key, ImageBaseType::RegionType region, unsigned int idx = 0);

   ImageBaseType::RegionType GetImageRequestedRegion(const std::string& key, unsigned int idx = 0);

   ImageMetadata &GetImageMetadata(const std::string& key, unsigned int idx = 0);
   void SetImageMetadata(const ImageMetadata & imd, const std::string& key, unsigned int idx = 0);

   itk::MetaDataDictionary GetMetadataDictionary(const std::string& key, unsigned int idx = 0);

   ImagePixelType GetImageBasePixelType(const std::string& key, unsigned int idx = 0);

   ImageBaseType* GetParameterImageBase(const std::string& key, unsigned int idx = 0);

   void SetParameterImageBase(const std::string& key, ImageBaseType* img, unsigned int idx = 0);

   void RegisterPipeline();

   void FreeResources();

   bool IsExecuteDone();

   bool IsMultiWritingEnabled();

 protected:
   Application();

   ~Application() override;

   /* Register a ProcessObject as a new progress source */
   void AddProcess(itk::ProcessObject* object, std::string description);

   void AddChoice(std::string const& paramKey, std::string const& paramName);

   void AddParameter(ParameterType type, std::string const& paramKey, std::string const& paramName);

   void AddRAMParameter(std::string const& paramKey = "ram");

   void AddRAMParameter(std::string const& paramKey, std::string const& paramName, unsigned int defaultValue);

   void AddRANDParameter(std::string const& paramKey = "rand");

   void AddRANDParameter(std::string const& paramKey, std::string const& paramName, unsigned int defaultValue);

   void ClearChoices(std::string const& key);

   std::vector<int> GetSelectedItems(std::string const& paramKey);

   void MandatoryOn(std::string const& paramKey);

   void MandatoryOff(std::string const& paramKey);

   /* Set the user level of access to a parameter */
   void SetParameterUserLevel(std::string const& paramKey, UserLevel level);

   /*  Set the parameter role (input/output) */
   void SetParameterRole(std::string const& paramKey, Role role);

   /* Get an image value
    *
    * Can be called for types :
    * \li ParameterType_InputImage
    */
   template <class TImageType>
   TImageType* GetParameterImage(std::string const& parameter);

   void AutomaticValueOn(std::string const& paramKey);

   void AutomaticValueOff(std::string const& paramKey);

   /* Set an output image value
    *
    * Can be called for types :
    * \li ParameterType_OutputImage
    */
   template <class TImageType>
   void SetParameterOutputImage(std::string const& parameter, TImageType* value);

   itkSetMacro(MultiWriting, bool);

   /* Enable in-application prevention of modifications to m_UserValue (default behaviour) */
   void EnableInPrivateDo();

   /* Disable in-application prevention of modifications to m_UserValue */
   void DisableInPrivateDo();

 private:
   /* Implement this method to add parameters */
   virtual void DoInit() = 0;

   /* Implement this method to update non valued parameters */
   virtual void DoUpdateParameters() = 0;

   /* Implement this method to build the output */
   virtual void DoExecute() = 0;

   /* This method will be called after the
    * ExecuteAndWriteOutput() call to allow for cleanup. Default
    * implementation does nothing */
   virtual void AfterExecuteAndWriteOutputs();

   virtual void DoFreeResources(){};

   Application(const Application&) = delete;
   void operator=(const Application&) = delete;

   std::string             m_Name;
   std::string             m_Description;
   ParameterGroup::Pointer m_ParameterList;
   otb::Logger::Pointer    m_Logger;

   itk::ProcessObject::Pointer m_ProgressSource;
   std::string                 m_ProgressSourceDescription;

   std::set<itk::ProcessObject::Pointer> m_Filters;

   std::string m_DocLongDescription;

   DocExampleStructure::Pointer m_DocExample;

   std::string m_DocAuthors;

   std::string m_DocLimitations;

   std::string m_DocSeeAlso;

   std::vector<std::string> m_DocTags;

   std::string m_Doclink;

   otb::Stopwatch m_Chrono;

   bool m_IsInPrivateDo;

   bool m_ExecuteDone;

   bool m_MultiWriting;

   friend class MapProjectionParametersHandler;
   friend class ElevationParametersHandler;

 }; // end class

 } // end namespace Wrapper
 } // end namespace otb


 #ifndef OTB_MANUAL_INSTANTIATION
 #include "otbWrapperApplication.hxx"
 #endif


 namespace otb
 {
 namespace Wrapper
 {

 extern template OTBApplicationEngine_EXPORT_TEMPLATE UInt8VectorImageType* Application::GetParameterImage<UInt8VectorImageType>(std::string const&);
 extern template OTBApplicationEngine_EXPORT_TEMPLATE Int16VectorImageType* Application::GetParameterImage<Int16VectorImageType>(std::string const&);
 extern template OTBApplicationEngine_EXPORT_TEMPLATE UInt16VectorImageType* Application::GetParameterImage<UInt16VectorImageType>(std::string const&);
 extern template OTBApplicationEngine_EXPORT_TEMPLATE Int32VectorImageType* Application::GetParameterImage<Int32VectorImageType>(std::string const&);
 extern template OTBApplicationEngine_EXPORT_TEMPLATE UInt32VectorImageType* Application::GetParameterImage<UInt32VectorImageType>(std::string const&);

 extern template OTBApplicationEngine_EXPORT_TEMPLATE FloatVectorImageType* Application::GetParameterImage<FloatVectorImageType>(std::string const&);
 extern template OTBApplicationEngine_EXPORT_TEMPLATE DoubleVectorImageType* Application::GetParameterImage<DoubleVectorImageType>(std::string const&);

 extern template OTBApplicationEngine_EXPORT_TEMPLATE ComplexInt16VectorImageType*
                                                      Application::GetParameterImage<ComplexInt16VectorImageType>(std::string const&);
 extern template OTBApplicationEngine_EXPORT_TEMPLATE ComplexInt32VectorImageType*
                                                      Application::GetParameterImage<ComplexInt32VectorImageType>(std::string const&);

 extern template OTBApplicationEngine_EXPORT_TEMPLATE ComplexFloatVectorImageType*
                                                      Application::GetParameterImage<ComplexFloatVectorImageType>(std::string const&);
 extern template OTBApplicationEngine_EXPORT_TEMPLATE ComplexDoubleVectorImageType*
                                                      Application::GetParameterImage<ComplexDoubleVectorImageType>(std::string const&);

 extern template OTBApplicationEngine_EXPORT_TEMPLATE UInt8RGBImageType* Application::GetParameterImage<UInt8RGBImageType>(std::string const&);
 extern template OTBApplicationEngine_EXPORT_TEMPLATE UInt8RGBAImageType* Application::GetParameterImage<UInt8RGBAImageType>(std::string const&);

 extern template OTBApplicationEngine_EXPORT_TEMPLATE UInt8ImageType* Application::GetParameterImage<UInt8ImageType>(std::string const&);
 extern template OTBApplicationEngine_EXPORT_TEMPLATE Int16ImageType* Application::GetParameterImage<Int16ImageType>(std::string const&);
 extern template OTBApplicationEngine_EXPORT_TEMPLATE UInt16ImageType* Application::GetParameterImage<UInt16ImageType>(std::string const&);
 extern template OTBApplicationEngine_EXPORT_TEMPLATE Int32ImageType* Application::GetParameterImage<Int32ImageType>(std::string const&);
 extern template OTBApplicationEngine_EXPORT_TEMPLATE UInt32ImageType* Application::GetParameterImage<UInt32ImageType>(std::string const&);

 extern template OTBApplicationEngine_EXPORT_TEMPLATE FloatImageType* Application::GetParameterImage<FloatImageType>(std::string const&);
 extern template OTBApplicationEngine_EXPORT_TEMPLATE DoubleImageType* Application::GetParameterImage<DoubleImageType>(std::string const&);

 extern template OTBApplicationEngine_EXPORT_TEMPLATE ComplexInt16ImageType* Application::GetParameterImage<ComplexInt16ImageType>(std::string const&);
 extern template OTBApplicationEngine_EXPORT_TEMPLATE ComplexInt32ImageType* Application::GetParameterImage<ComplexInt32ImageType>(std::string const&);

 extern template OTBApplicationEngine_EXPORT_TEMPLATE ComplexFloatImageType* Application::GetParameterImage<ComplexFloatImageType>(std::string const&);
 extern template OTBApplicationEngine_EXPORT_TEMPLATE ComplexDoubleImageType* Application::GetParameterImage<ComplexDoubleImageType>(std::string const&);

 //

 extern template OTBApplicationEngine_EXPORT_TEMPLATE void Application::SetParameterOutputImage<UInt8VectorImageType>(std::string const&, UInt8VectorImageType*);
 extern template OTBApplicationEngine_EXPORT_TEMPLATE void Application::SetParameterOutputImage<Int16VectorImageType>(std::string const&, Int16VectorImageType*);
 extern template OTBApplicationEngine_EXPORT_TEMPLATE void Application::SetParameterOutputImage<UInt16VectorImageType>(std::string const&,
                                                                                                                       UInt16VectorImageType*);
 extern template OTBApplicationEngine_EXPORT_TEMPLATE void Application::SetParameterOutputImage<Int32VectorImageType>(std::string const&, Int32VectorImageType*);
 extern template OTBApplicationEngine_EXPORT_TEMPLATE void Application::SetParameterOutputImage<UInt32VectorImageType>(std::string const&,
                                                                                                                       UInt32VectorImageType*);

 extern template OTBApplicationEngine_EXPORT_TEMPLATE void Application::SetParameterOutputImage<FloatVectorImageType>(std::string const&, FloatVectorImageType*);
 extern template OTBApplicationEngine_EXPORT_TEMPLATE void Application::SetParameterOutputImage<DoubleVectorImageType>(std::string const&,
                                                                                                                       DoubleVectorImageType*);

 extern template OTBApplicationEngine_EXPORT_TEMPLATE void Application::SetParameterOutputImage<ComplexInt16VectorImageType>(std::string const&,
                                                                                                                             ComplexInt16VectorImageType*);
 extern template OTBApplicationEngine_EXPORT_TEMPLATE void Application::SetParameterOutputImage<ComplexInt32VectorImageType>(std::string const&,
                                                                                                                             ComplexInt32VectorImageType*);

 extern template OTBApplicationEngine_EXPORT_TEMPLATE void Application::SetParameterOutputImage<ComplexFloatVectorImageType>(std::string const&,
                                                                                                                             ComplexFloatVectorImageType*);
 extern template OTBApplicationEngine_EXPORT_TEMPLATE void Application::SetParameterOutputImage<ComplexDoubleVectorImageType>(std::string const&,
                                                                                                                              ComplexDoubleVectorImageType*);

 extern template OTBApplicationEngine_EXPORT_TEMPLATE void Application::SetParameterOutputImage<UInt8RGBImageType>(std::string const&, UInt8RGBImageType*);
 extern template OTBApplicationEngine_EXPORT_TEMPLATE void Application::SetParameterOutputImage<UInt8RGBAImageType>(std::string const&, UInt8RGBAImageType*);

 extern template OTBApplicationEngine_EXPORT_TEMPLATE void Application::SetParameterOutputImage<UInt8ImageType>(std::string const&, UInt8ImageType*);
 extern template OTBApplicationEngine_EXPORT_TEMPLATE void Application::SetParameterOutputImage<Int16ImageType>(std::string const&, Int16ImageType*);
 extern template OTBApplicationEngine_EXPORT_TEMPLATE void Application::SetParameterOutputImage<UInt16ImageType>(std::string const&, UInt16ImageType*);
 extern template OTBApplicationEngine_EXPORT_TEMPLATE void Application::SetParameterOutputImage<Int32ImageType>(std::string const&, Int32ImageType*);
 extern template OTBApplicationEngine_EXPORT_TEMPLATE void Application::SetParameterOutputImage<UInt32ImageType>(std::string const&, UInt32ImageType*);

 extern template OTBApplicationEngine_EXPORT_TEMPLATE void Application::SetParameterOutputImage<FloatImageType>(std::string const&, FloatImageType*);
 extern template OTBApplicationEngine_EXPORT_TEMPLATE void Application::SetParameterOutputImage<DoubleImageType>(std::string const&, DoubleImageType*);

 extern template OTBApplicationEngine_EXPORT_TEMPLATE void Application::SetParameterOutputImage<ComplexInt16ImageType>(std::string const&,
                                                                                                                       ComplexInt16ImageType*);
 extern template OTBApplicationEngine_EXPORT_TEMPLATE void Application::SetParameterOutputImage<ComplexInt32ImageType>(std::string const&,
                                                                                                                       ComplexInt32ImageType*);

 extern template OTBApplicationEngine_EXPORT_TEMPLATE void Application::SetParameterOutputImage<ComplexFloatImageType>(std::string const&,
                                                                                                                       ComplexFloatImageType*);
 extern template OTBApplicationEngine_EXPORT_TEMPLATE void Application::SetParameterOutputImage<ComplexDoubleImageType>(std::string const&,
                                                                                                                        ComplexDoubleImageType*);

 } // namespace Wrapper
 } // namespace otb


 #endif // otbWrapperApplication_h_