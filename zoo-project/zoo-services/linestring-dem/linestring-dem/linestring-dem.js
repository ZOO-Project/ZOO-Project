const gdal = require('gdal-async');

function linestringDem(conf, inputs, outputs) {
  const flightJSON = JSON.parse(inputs.Line.value);
  const flight = gdal.Geometry.fromGeoJson(flightJSON.features[0].geometry);
  const demDS = gdal.open(inputs.DEM.value || 'topofr.tif');
  const xform = new gdal.CoordinateTransformation(gdal.SpatialReference.fromEPSG(4326), demDS);
  const dem = demDS.bands.get(1);

  const heights = [];

  const points = flight.points.toArray();

  for (const p of points) {
    const d = xform.transformPoint(p);
    const h = dem.pixels.get(d.x, d.y);
    heights.push(h);
  }

  outputs.result.value = JSON.stringify(heights);

  return SERVICE_SUCCEEDED;
}

module.exports = linestringDem;
