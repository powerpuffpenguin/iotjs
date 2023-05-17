(function () {
var exports={};
    var __extends=this&&this.__extends||function(){var c=function(a,b){c=Object.setPrototypeOf||{__proto__:[]}instanceof Array&&function(d,e){d.__proto__=e}||function(d,e){for(var f in e)Object.prototype.hasOwnProperty.call(e,f)&&(d[f]=e[f])};return c(a,b)};return function(a,b){function d(){this.constructor=a}if("function"!==typeof b&&null!==b)throw new TypeError("Class extends value "+String(b)+" is not a constructor or null");c(a,b);a.prototype=null===b?Object.create(b):(d.prototype=b.prototype,new d)}}();
    Object.defineProperty(exports,"__esModule",{value:!0});exports.IotError=exports.Completer=void 0;
    var Completer=function(){function c(){var a=this;this.c_=!1;this.promise_=new Promise(function(b,d){a.resolve_=b;a.reject_=d})}Object.defineProperty(c.prototype,"isCompleted",{get:function(){return this.c_},enumerable:!1,configurable:!0});Object.defineProperty(c.prototype,"promise",{get:function(){return this.promise_},enumerable:!1,configurable:!0});c.prototype.resolve=function(a){this.c_||(this.c_=!0,this.resolve_&&this.resolve_(a))};c.prototype.reject=function(a){this.c_||(this.c_=!0,this.reject_&&
    this.reject_(a))};return c}();exports.Completer=Completer;var IotError=function(c){function a(b,d){var e=c.call(this,b,d)||this;e.code=0;return e}__extends(a,c);return a}(Error);exports.IotError=IotError;
return exports;
})()

