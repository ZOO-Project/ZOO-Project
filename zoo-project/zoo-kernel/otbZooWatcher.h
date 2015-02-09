/*=========================================================================

  Copyright (c) Centre National d'Etudes Spatiales. All rights reserved.
  See OTBCopyright.txt for details.

  Some parts of this code are derived from ITK. See ITKCopyright.txt
  for details.


     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notices for more information.

  See Ref: http://hg.orfeo-toolbox.org/OTB/ Copyright

=========================================================================*/
#ifndef __zooOtbWatcher_h
#define __zooOtbWatcher_h

#include "otbFilterWatcherBase.h"
#include "service.h"

class /*ITK_EXPORT*/ ZooWatcher : public otb::FilterWatcherBase
{
public:

  /** Constructor. Takes a ProcessObject to monitor and an optional
   * comment string that is prepended to each event message. */
  ZooWatcher(itk::ProcessObject* process,
                        const char *comment = "");

  ZooWatcher(itk::ProcessObject* process,
                        const std::string& comment = "");

  /** Default constructor */
  ZooWatcher();

  /** Get/Set/Free Configuration maps */
  void SetConf(maps **conf)
  {
    m_Conf=dupMaps(conf);
  }
  const maps& GetConf() const
  {
    return *m_Conf;
  }
  void FreeConf(){
    freeMaps(&m_Conf);
    free(m_Conf);
  }
protected:

  /** Callback method to show the ProgressEvent */
  virtual void ShowProgress();

  /** Callback method to show the StartEvent */
  virtual void StartFilter();

  /** Callback method to show the EndEvent */
  virtual void EndFilter();

private:

  /** Main conf maps */
  maps* m_Conf;

};

#endif
