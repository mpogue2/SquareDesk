/*

    Copyright 2017 Brad Christie

    This file is part of Taminations.

    Taminations is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as published
    by the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Taminations is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with Taminations.  If not, see <http://www.gnu.org/licenses/>.

 */
//  Color class
define(function() {

  Color = function(r,g,b)
  {
    this.r = Math.floor(r);
    this.g = Math.floor(g);
    this.b = Math.floor(b);
  };
  Color.FACTOR = 0.7;  // darker() factor from Java
  Color.hex = [ '0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f'];
  Color.black = new Color(0,0,0);
  Color.red = new Color(255,0,0);
  Color.green = new Color(0,255,0);
  Color.blue = new Color(0,0,255);
  Color.yellow = new Color(255,255,0);
  Color.orange = new Color(255,200,0);
  Color.lightGray = new Color(192,192,192);
  Color.gray = new Color(128,128,128);
  Color.magenta = new Color(255,0,255);
  Color.cyan = new Color(0,255,255);

  Color.prototype.invert = function()
  {
    return new Color(255-this.r,255-this.g,255-this.b);
  };

  Color.prototype.darker = function()
  {
    return new Color(Math.floor(this.r*Color.FACTOR),
        Math.floor(this.g*Color.FACTOR),
        Math.floor(this.b*Color.FACTOR));
  };

  Color.prototype.brighter = function()
  {
    return this.invert().darker().invert();
  };

  Color.prototype.veryBright = function()
  {
    return this.brighter().brighter().brighter().brighter();
  };

  Color.prototype.rotate = function()
  {
    var cc = new Color(0,0,0);
    if (this.r == 255 && this.g == 0 && this.b == 0)
      cc.g = c.b = 255;
    else if (this.r == Color.lightGray.r)
      cc = Color.lightGray;
    else
      cc.b = 255;
    return cc;
  };

  Color.prototype.toString = function()
  {
    return '#' + Color.hex[this.r>>4] + Color.hex[this.r&0xf] +
    Color.hex[this.g>>4] + Color.hex[this.g&0xf] +
    Color.hex[this.b>>4] + Color.hex[this.b&0xf];
  };

  //  Return value for define function
  return Color;
});
