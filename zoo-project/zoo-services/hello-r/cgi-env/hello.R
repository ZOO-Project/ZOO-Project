#
# Author : Gérald FENOY
#
# Copyright 2019-2023 GeoLabs SARL. All rights reserved.
# 
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

source("minimal.r")

hellor <- function(a,b,c) {
    # Create a useless graph
    cars <- c(1,2,3,4,5)
    filePath <- paste(a[["main"]][["tmpPath"]],paste("cars",zoo[["conf"]][["lenv"]][["usid"]],".png",sep=""),sep="/")
    png(filePath)
    plot(cars)
    quartzFonts(avenir = c("Avenir Book", "Avenir Black", "Avenir Book Oblique", "Avenir Black Oblique"))
    par(family = 'avenir')
    title("Hello from R World!")
    dev.off()
    # Set the result
    zoo[["outputs"]][["Result"]][["value"]] <<- paste("Hello",b[["S"]][["value"]],"from the R World! (plot ref:",filePath,")",sep=" ")
    # Update the main conf settings
    zoo[["conf"]][["lenv"]][["message"]] <<- "Running from R world!"
    # Return SERVICE_SUCCEEDEED
    return(zoo[["SERVICE_SUCCEEDEED"]])
}

failR <- function(conf,inputs,outputs){
    zoo[["conf"]][["lenv"]][["message"]] <<- ZOOTranslate("Failed running from R world!")
    return(zoo[["SERVICE_FAILED"]])
}

voronoipolygons <- function(x,poly) {
  require(deldir)
  if (.hasSlot(x, 'coords')) {
    crds <- x@coords  
  } else crds <- x
  bb = bbox(poly)
  rw = as.numeric(t(bb))
  z <- deldir(crds[,1], crds[,2],rw=rw)
  w <- tile.list(z)
  polys <- vector(mode='list', length=length(w)) 
  require(sp)
  finalCoordinates <- c()
  for (i in seq(along=polys)) {
    pcrds <- cbind(w[[i]]$x, w[[i]]$y)
    pcrds <- rbind(pcrds, pcrds[1,])
    finalCoordinates[[i]] <- i
    polys[[i]] <- Polygons(list(Polygon(pcrds)), ID=as.character(i))
  }
  SP <- SpatialPolygons(polys)
  voronoi <- SpatialPolygonsDataFrame(SP, data=data.frame(identifier=finalCoordinates))
  return(voronoi)
}

RVoronoi <- function(conf,inputs,outputs){
    require(rgdal)
    require(rgeos)
    message(names(inputs[["InputPoints"]]))
    message(inputs[["InputPoints"]])
    myGeoData <- readOGR(inputs[["InputPoints"]][["cache_file"]],layer=ogrListLayers(inputs[["InputPoints"]][["cache_file"]])[1], verbose = FALSE);
    voronoiData <- voronoipolygons(myGeoData,myGeoData)
    filePath <- paste(conf[["main"]][["tmpPath"]],paste("RBuffer_",zoo[["conf"]][["lenv"]][["usid"]],".shp",sep=""),sep="/")
    writeOGR(voronoiData, dsn=filePath, layer="Voronoi", driver="ESRI Shapefile", verbose = FALSE)
    zoo[["outputs"]][["Result"]][["storage"]] <<- filePath
    return(zoo[["SERVICE_SUCCEEDEED"]])
}

