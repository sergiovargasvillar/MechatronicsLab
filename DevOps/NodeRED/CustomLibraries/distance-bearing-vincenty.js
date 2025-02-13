async function initialBearingTo(points) {
  const latlonSpherical = await import("geodesy/latlon-ellipsoidal-vincenty.js");
  latlon = latlonSpherical.default;
  const p1 = await new latlon(points.init.lat, points.init.lon);
  const p2 = await new latlon(points.final.lat, points.final.lon);
  const initialBearing = p1.initialBearingTo(p2);
  const distance = p1.distanceTo(p2);
  const bearingDistanceTo = {
    bearing: initialBearing,
    distance: distance
  };
  return bearingDistanceTo;
}

module.exports = function (RED) {
  function bearing(config) {
    RED.nodes.createNode(this, config);
    var node = this;
    node.on("input", function (msg) {
      let bearing = initialBearingTo(msg.payload).then(function (distance) {
        msg.payload = distance;
        console.log("--------[Distance Vincenty OUTPUT]--------");
        console.log(msg.payload);
        node.send(msg);
        return distance;
      });
    });
  }
  RED.nodes.registerType("distance-bearing-vincenty", bearing);
};
