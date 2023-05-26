const gdal = require('gdal-async');
const path = require('path');

function linestringDem(conf, inputs, outputs) {
  try {
    // Get the input geometry
    const json = JSON.parse(inputs.Geometry.value);
    let track;
    if (json.type === 'FeatureCollection')
      track = gdal.Geometry.fromGeoJson(json.features[0].geometry);
    else
      track = gdal.Geometry.fromGeoJson(json);

    // Open the DEM file
    const demFile = inputs.RasterFile.value || 'topofr.tif';
    const demPath = path.resolve(conf.main.dataPath, demFile);
    const demDS = gdal.open(demPath);

    // Create a transform between WGS84 and the DEM pixel coordinates
    const xform = new gdal.CoordinateTransformation(gdal.SpatialReference.fromEPSG(4326), demDS);
    const dem = demDS.bands.get(1);

    const heights = new gdal.LineString();

    track.segmentize(0.1);
    const points = track.points.toArray();
    for (const p of points) {
      const d = xform.transformPoint(p);
      const h = dem.pixels.get(d.x, d.y);
      heights.points.add(new gdal.Point(p.x, p.y, h));
    }

    outputs.Profile.value = JSON.stringify(heights.toObject());
    // This is for compatibility with GdalExtractProfile
    outputs.Profile.mimeType = 'text/plain';
    console.error('len is ' + outputs.Profile.value.length);

    return SERVICE_SUCCEEDED;
  } catch (e) {
    alert(e);
    throw e;
  }
}

module.exports = { linestringDem };
