/***************************************************************************
                           qgspoint.h  -  description
                              -------------------
     begin                : Sat Jun 22 2002
     copyright            : (C) 2002 by Gary E.Sherman
     email                : sherman at mrcc.com
  ***************************************************************************/
  
 /***************************************************************************
  *                                                                         *
  *   This program is free software; you can redistribute it and/or modify  *
  *   it under the terms of the GNU General Public License as published by  *
  *   the Free Software Foundation; either version 2 of the License, or     *
  *   (at your option) any later version.                                   *
  *                                                                         *
  ***************************************************************************/
  
 #ifndef QGSPOINTXY_H
 #define QGSPOINTXY_H
  
 #include "qgis_core.h"
 #include "qgsvector.h"
  
 #include "qgis.h"
  
 #include <iostream>
 #include <QString>
 #include <QPoint>
 #include <QObject>
  
 class QgsPoint;
  
 class CORE_EXPORT QgsPointXY
 {
     Q_GADGET
  
     Q_PROPERTY( double x READ x WRITE setX )
     Q_PROPERTY( double y READ y WRITE setY )
  
   public:
     QgsPointXY() = default;
  
     QgsPointXY( const QgsPointXY &p ) SIP_HOLDGIL;
  
     QgsPointXY( double x, double y ) SIP_HOLDGIL
   : mX( x )
     , mY( y )
     , mIsEmpty( false )
     {}
  
     QgsPointXY( QPointF point ) SIP_HOLDGIL
   : mX( point.x() )
     , mY( point.y() )
     , mIsEmpty( false )
     {}
  
     QgsPointXY( QPoint point ) SIP_HOLDGIL
   : mX( point.x() )
     , mY( point.y() )
     , mIsEmpty( false )
     {}
  
     QgsPointXY( const QgsPoint &point ) SIP_HOLDGIL;
  
     // IMPORTANT - while QgsPointXY is inherited by QgsReferencedPointXY, we do NOT want a virtual destructor here
     // because this class MUST be lightweight and we don't want the cost of the vtable here.
     // see https://github.com/qgis/QGIS/pull/4720#issuecomment-308652392
     ~QgsPointXY() = default;
  
     void setX( double x ) SIP_HOLDGIL
     {
       mX = x;
       mIsEmpty = false;
     }
  
     void setY( double y ) SIP_HOLDGIL
     {
       mY = y;
       mIsEmpty = false;
     }
  
     void set( double x, double y ) SIP_HOLDGIL
     {
       mX = x;
       mY = y;
       mIsEmpty = false;
     }
  
     double x() const SIP_HOLDGIL
     {
       return mX;
     }
  
     double y() const SIP_HOLDGIL
     {
       return mY;
     }
  
     QPointF toQPointF() const
     {
       return QPointF( mX, mY );
     }
  
     QString toString( int precision = -1 ) const;
  
     QString asWkt() const;
  
     double sqrDist( double x, double y ) const SIP_HOLDGIL
     {
       return ( mX - x ) * ( mX - x ) + ( mY - y ) * ( mY - y );
     }
  
     double sqrDist( const QgsPointXY &other ) const SIP_HOLDGIL
     {
       return sqrDist( other.x(), other.y() );
     }
  
     double distance( double x, double y ) const SIP_HOLDGIL
     {
       return std::sqrt( sqrDist( x, y ) );
     }
  
     double distance( const QgsPointXY &other ) const SIP_HOLDGIL
     {
       return std::sqrt( sqrDist( other ) );
     }
  
     double sqrDistToSegment( double x1, double y1, double x2, double y2, QgsPointXY &minDistPoint SIP_OUT, double epsilon = DEFAULT_SEGMENT_EPSILON ) const SIP_HOLDGIL;
  
     double azimuth( const QgsPointXY &other ) const SIP_HOLDGIL;
  
     QgsPointXY project( double distance, double bearing ) const SIP_HOLDGIL;
  
     bool isEmpty() const SIP_HOLDGIL { return mIsEmpty; }
  
     bool compare( const QgsPointXY &other, double epsilon = 4 * std::numeric_limits<double>::epsilon() ) const SIP_HOLDGIL
     {
       return ( qgsDoubleNear( mX, other.x(), epsilon ) && qgsDoubleNear( mY, other.y(), epsilon ) );
     }
  
     bool operator==( const QgsPointXY &other ) SIP_HOLDGIL
     {
       if ( isEmpty() && other.isEmpty() )
         return true;
       if ( isEmpty() && !other.isEmpty() )
         return false;
       if ( ! isEmpty() && other.isEmpty() )
         return false;
  
       bool equal = true;
       equal &= qgsDoubleNear( other.x(), mX, 1E-8 );
       equal &= qgsDoubleNear( other.y(), mY, 1E-8 );
  
       return equal;
     }
  
     bool operator!=( const QgsPointXY &other ) const SIP_HOLDGIL
     {
       if ( isEmpty() && other.isEmpty() )
         return false;
       if ( isEmpty() && !other.isEmpty() )
         return true;
       if ( ! isEmpty() && other.isEmpty() )
         return true;
  
       bool equal = true;
       equal &= qgsDoubleNear( other.x(), mX, 1E-8 );
       equal &= qgsDoubleNear( other.y(), mY, 1E-8 );
  
       return !equal;
     }
  
     void multiply( double scalar ) SIP_HOLDGIL
     {
       mX *= scalar;
       mY *= scalar;
     }
  
     QgsPointXY &operator=( const QgsPointXY &other ) SIP_HOLDGIL
     {
       if ( &other != this )
       {
         mX = other.x();
         mY = other.y();
         mIsEmpty = other.isEmpty();
       }
  
       return *this;
     }
  
     QgsVector operator-( const QgsPointXY &p ) const { return QgsVector( mX - p.mX, mY - p.mY ); }
  
     QgsPointXY &operator+=( QgsVector v ) { *this = *this + v; return *this; }
  
     QgsPointXY &operator-=( QgsVector v ) { *this = *this - v; return *this; }
  
     QgsPointXY operator+( QgsVector v ) const { return QgsPointXY( mX + v.x(), mY + v.y() ); }
  
     QgsPointXY operator-( QgsVector v ) const { return QgsPointXY( mX - v.x(), mY - v.y() ); }
  
     QgsPointXY operator*( double scalar ) const { return QgsPointXY( mX * scalar, mY * scalar ); }
  
     QgsPointXY operator/( double scalar ) const { return QgsPointXY( mX / scalar, mY / scalar ); }
  
     QgsPointXY &operator*=( double scalar ) { mX *= scalar; mY *= scalar; return *this; }
  
     QgsPointXY &operator/=( double scalar ) { mX /= scalar; mY /= scalar; return *this; }
  
     operator QVariant() const
     {
       return QVariant::fromValue( *this );
     }
  
 #ifdef SIP_RUN
     SIP_PYOBJECT __repr__();
     % MethodCode
     QString str = QStringLiteral( "<QgsPointXY: %1>" ).arg( sipCpp->asWkt() );
     sipRes = PyUnicode_FromString( str.toUtf8().constData() );
     % End
  
     int __len__();
     % MethodCode
     sipRes = 2;
     % End
  
  
     SIP_PYOBJECT __getitem__( int );
     % MethodCode
     if ( a0 == 0 )
     {
       sipRes = Py_BuildValue( "d", sipCpp->x() );
     }
     else if ( a0 == 1 )
     {
       sipRes = Py_BuildValue( "d", sipCpp->y() );
     }
     else
     {
       QString msg = QString( "Bad index: %1" ).arg( a0 );
       PyErr_SetString( PyExc_IndexError, msg.toLatin1().constData() );
     }
     % End
  
     long __hash__() const;
     % MethodCode
     sipRes = qHash( *sipCpp );
     % End
 #endif
  
   private:
  
     double mX = 0; //std::numeric_limits<double>::quiet_NaN();
  
     double mY = 0; //std::numeric_limits<double>::quiet_NaN();
  
     bool mIsEmpty = true;
  
     friend uint qHash( const QgsPointXY &pnt );
  
 }; // class QgsPointXY
  
 Q_DECLARE_METATYPE( QgsPointXY )
  
 inline bool operator==( const QgsPointXY &p1, const QgsPointXY &p2 ) SIP_SKIP
 {
   const bool nan1X = std::isnan( p1.x() );
   const bool nan2X = std::isnan( p2.x() );
   if ( nan1X != nan2X )
     return false;
   if ( !nan1X && !qgsDoubleNear( p1.x(), p2.x(), 1E-8 ) )
     return false;
  
   const bool nan1Y = std::isnan( p1.y() );
   const bool nan2Y = std::isnan( p2.y() );
   if ( nan1Y != nan2Y )
     return false;
  
   if ( !nan1Y && !qgsDoubleNear( p1.y(), p2.y(), 1E-8 ) )
     return false;
  
   return true;
 }
  
 inline std::ostream &operator << ( std::ostream &os, const QgsPointXY &p ) SIP_SKIP
 {
   // Use Local8Bit for printouts
   os << p.toString().toLocal8Bit().data();
   return os;
 }
  
 inline uint qHash( const QgsPointXY &p ) SIP_SKIP
 {
   uint hash;
   const uint h1 = qHash( static_cast< quint64 >( p.mX ) );
   const uint h2 = qHash( static_cast< quint64 >( p.mY ) );
   hash = h1 ^ ( h2 << 1 );
   return hash;
 }
  
  
 #endif //QGSPOINTXY_H