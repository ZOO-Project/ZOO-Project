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
#include "otbZooWatcher.h"
#include "otbConfigure.h"
#include "service_internal.h"

ZooWatcher
::ZooWatcher()
{
}

ZooWatcher
::ZooWatcher(itk::ProcessObject* process,
	     const char *comment)
  : otb::FilterWatcherBase(process, comment)
{
}

ZooWatcher
::ZooWatcher(itk::ProcessObject* process,
	     const std::string& comment)
  : otb::FilterWatcherBase(process, comment.c_str())
{
}

void
ZooWatcher
::ShowProgress()
{
  if (m_Process)
    {
      int progressPercent = static_cast<int>(m_Process->GetProgress() * 100);
      updateStatus(m_Conf,progressPercent,m_Comment.c_str());
    }
}

void
ZooWatcher
::StartFilter()
{
#if OTB_VERSION_MAJOR < 6 
  m_TimeProbe.Start();
#endif
}

void
ZooWatcher
::EndFilter()
{
#if OTB_VERSION_MAJOR < 6 
  m_TimeProbe.Stop();
  std::ostringstream elapsedTime;
  elapsedTime.precision(1);
  elapsedTime << m_TimeProbe.GetMean();

  std::cerr << " (OTB Execution: "
            << elapsedTime.str()
            << " seconds)"
            << std::endl;
#endif
}
