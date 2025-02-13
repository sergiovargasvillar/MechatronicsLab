const π = Math.PI; //15 decimals
// Extend Number object with methods to convert between degrees & radians
Number.prototype.toRadians = function () {
  return (this * Math.PI) / 180;
};
Number.prototype.toDegrees = function () {
  return (this * 180) / Math.PI;
};

async function destinationPoint(geodesyPoint) {
  const latlonSpherical = await import(
    "geodesy/latlon-ellipsoidal-vincenty.js"
  );
  const datums = await import("geodesy/latlon-ellipsoidal-datum.js");
  data = datums.default;
  latlon = latlonSpherical.default;
  let result = [];
  geodesyPoint.forEach(async function (data, index) {
    const p1 = await new latlon(data.lat, data.long);
   // p1.datum = await data.datums.WGS84;
    const distance = Math.sqrt(Math.pow(data.x, 2) + Math.pow(data.y, 2));
    let brng = Math.atan2(data.x, data.y); // θ is the bearing (clockwise from north)
    brng = brng.toDegrees();
    const p2 = p1.direct(distance, brng);
    console.log(p2);
    data.lat = p2.point._lat;
    data.long = p2.point._lon;
    result.push(data);
  });
  return result;
}

module.exports = function (RED) {
  function LatLonVincenty(config) {
    RED.nodes.createNode(this, config);
    var node = this;
    node.on("input", function (msg) {
      destinationPoint(msg.payload).then(function (result) {
        msg.payload = result;
        node.send(msg);
        return result;
      });
    });
  }
  RED.nodes.registerType("latlon-Vincenty", LatLonVincenty);
};
