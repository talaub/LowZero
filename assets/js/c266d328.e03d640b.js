"use strict";(self.webpackChunkmy_website=self.webpackChunkmy_website||[]).push([[9284],{3905:(e,t,n)=>{n.d(t,{Zo:()=>p,kt:()=>g});var i=n(7294);function o(e,t,n){return t in e?Object.defineProperty(e,t,{value:n,enumerable:!0,configurable:!0,writable:!0}):e[t]=n,e}function r(e,t){var n=Object.keys(e);if(Object.getOwnPropertySymbols){var i=Object.getOwnPropertySymbols(e);t&&(i=i.filter((function(t){return Object.getOwnPropertyDescriptor(e,t).enumerable}))),n.push.apply(n,i)}return n}function a(e){for(var t=1;t<arguments.length;t++){var n=null!=arguments[t]?arguments[t]:{};t%2?r(Object(n),!0).forEach((function(t){o(e,t,n[t])})):Object.getOwnPropertyDescriptors?Object.defineProperties(e,Object.getOwnPropertyDescriptors(n)):r(Object(n)).forEach((function(t){Object.defineProperty(e,t,Object.getOwnPropertyDescriptor(n,t))}))}return e}function s(e,t){if(null==e)return{};var n,i,o=function(e,t){if(null==e)return{};var n,i,o={},r=Object.keys(e);for(i=0;i<r.length;i++)n=r[i],t.indexOf(n)>=0||(o[n]=e[n]);return o}(e,t);if(Object.getOwnPropertySymbols){var r=Object.getOwnPropertySymbols(e);for(i=0;i<r.length;i++)n=r[i],t.indexOf(n)>=0||Object.prototype.propertyIsEnumerable.call(e,n)&&(o[n]=e[n])}return o}var l=i.createContext({}),d=function(e){var t=i.useContext(l),n=t;return e&&(n="function"==typeof e?e(t):a(a({},t),e)),n},p=function(e){var t=d(e.components);return i.createElement(l.Provider,{value:t},e.children)},c="mdxType",m={inlineCode:"code",wrapper:function(e){var t=e.children;return i.createElement(i.Fragment,{},t)}},h=i.forwardRef((function(e,t){var n=e.components,o=e.mdxType,r=e.originalType,l=e.parentName,p=s(e,["components","mdxType","originalType","parentName"]),c=d(n),h=o,g=c["".concat(l,".").concat(h)]||c[h]||m[h]||r;return n?i.createElement(g,a(a({ref:t},p),{},{components:n})):i.createElement(g,a({ref:t},p))}));function g(e,t){var n=arguments,o=t&&t.mdxType;if("string"==typeof e||o){var r=n.length,a=new Array(r);a[0]=h;var s={};for(var l in t)hasOwnProperty.call(t,l)&&(s[l]=t[l]);s.originalType=e,s[c]="string"==typeof e?e:o,a[1]=s;for(var d=2;d<r;d++)a[d]=n[d];return i.createElement.apply(null,a)}return i.createElement.apply(null,n)}h.displayName="MDXCreateElement"},2331:(e,t,n)=>{n.r(t),n.d(t,{assets:()=>l,contentTitle:()=>a,default:()=>m,frontMatter:()=>r,metadata:()=>s,toc:()=>d});var i=n(7462),o=(n(7294),n(3905));const r={},a="Viewport",s={unversionedId:"widgets/viewportwidget",id:"widgets/viewportwidget",title:"Viewport",description:"The viewport widget is the main editing widget of the LowEngine editor.",source:"@site/docs/widgets/viewportwidget.mdx",sourceDirName:"widgets",slug:"/widgets/viewportwidget",permalink:"/LowZero/docs/next/widgets/viewportwidget",draft:!1,tags:[],version:"current",frontMatter:{},sidebar:"tutorialSidebar",previous:{title:"Scene",permalink:"/LowZero/docs/next/widgets/scenewidget"}},l={},d=[{value:"Moving the camera",id:"moving-the-camera",level:2},{value:"Selecting entities",id:"selecting-entities",level:2},{value:"Entering/exiting playmode",id:"enteringexiting-playmode",level:2},{value:"Editing entities",id:"editing-entities",level:2}],p={toc:d},c="wrapper";function m(e){let{components:t,...r}=e;return(0,o.kt)(c,(0,i.Z)({},p,r,{components:t,mdxType:"MDXLayout"}),(0,o.kt)("h1",{id:"viewport"},"Viewport"),(0,o.kt)("p",null,"The viewport widget is the main editing widget of the LowEngine editor.\nThis widget displays the 3D scene and allows the user to edit entities directly in 3D.\nWhen playtesting the game this widget will also be used to display the game while playing."),(0,o.kt)("p",null,"Therefore the viewport widget supports two modes: Editing and playing.\nThis section will mostly focus on the editing mode. Playmode is only used when playtesting the game in-editor."),(0,o.kt)("p",null,(0,o.kt)("img",{alt:"Viewport widget",src:n(9861).Z,width:"849",height:"534"})),(0,o.kt)("h2",{id:"moving-the-camera"},"Moving the camera"),(0,o.kt)("p",null,"This section explains how to move around the camera in edit mode.\nIt is possible to rotate the camera by holding down the right mouse button and moving around the mouse.\nThis controls similarly to first person games."),(0,o.kt)("p",null,"To actually move around the camera in 3D the WASD keys can be used.\nThe direction of movement will always be relative to the cameras orientation.\nIn order to be able to move up and down the Q and E keys can be used.\nThe mouse scrollwheel can be used to increase or decrease the camera's movement speed."),(0,o.kt)("h2",{id:"selecting-entities"},"Selecting entities"),(0,o.kt)("p",null,"It is possible to hover and select ",(0,o.kt)("a",{parentName:"p",href:"../entity"},"entities")," in 3D.\nHowever, this feature only works for ",(0,o.kt)("a",{parentName:"p",href:"../entity"},"entities")," that have the ",(0,o.kt)("a",{parentName:"p",href:"../components/meshrenderer"},"MeshRenderer")," component assigned to them."),(0,o.kt)("h2",{id:"enteringexiting-playmode"},"Entering/exiting playmode"),(0,o.kt)("p",null,"On the top of the viewport widget there is a play button that allows the user to enter playmode.\nEntering playmode removes all editing features like moving the camera and hovering/selecting entities.\nThis essentially just starts the game in-editor.\nTo exit playmode just use the same button that now displays a stop symbol.\nThis reloads all scenes and regions and puts the editor back in edit mode."),(0,o.kt)("h2",{id:"editing-entities"},"Editing entities"),(0,o.kt)("p",null,"Once an entity has been selected it is possible to move it around and edit its transformation values in 3D using gizmos.\nThis feature is only enabled in edit mode.\nThe selected entity will display translation (movement) gizmos at its center.\nThese arrows and faces can be grabbed with the mouse and used to move the entity around in 3D."),(0,o.kt)("p",null,"Other buttons in the top left of the viewport widget allow the user to switch to rotation and scaling gizmos.\nThese work similarly to the translation gizmos."),(0,o.kt)("admonition",{type:"note"},(0,o.kt)("p",{parentName:"admonition"},"For all of the different transformation options the white center point of the gizmos will be used as the pivot point for the transformation.")),(0,o.kt)("p",null,(0,o.kt)("img",{alt:"Viewport editing",src:n(2061).Z,width:"1273",height:"881"})))}m.isMDXComponent=!0},2061:(e,t,n)=>{n.d(t,{Z:()=>i});const i=n.p+"assets/images/viewport_editing-b9d36034b71689bf086224ebcae670e4.gif"},9861:(e,t,n)=>{n.d(t,{Z:()=>i});const i=n.p+"assets/images/viewportwidget-4520ca65042714fcc58a3a3b04ec93ac.PNG"}}]);