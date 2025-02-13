async function distance(points) {
  const latLonSph = await import("geodesy/latlon-spherical.js");
  def = latLonSph.default;
  const p1 = new def(points.init.lat, points.init.lon);
  const p2 = new def(points.final.lat, points.final.lon);
  const d = p1.distanceTo(p2);
  return d;
}

module.exports = function (RED) {
  function distanceToSpherical(config) {
    RED.nodes.createNode(this, config);
    var node = this;
    node.on("input", function (msg) {
      console.log(msg);
      distance(msg.payload).then(function (distance) {
        msg.payload = distance;
        console.log("--------[Distance Spherical OUTPUT]--------");
        console.log(msg.payload);
        node.send(msg);
        return distance;
      });
    });
  }
  RED.nodes.registerType("distance-spherical", distanceToSpherical);
};
