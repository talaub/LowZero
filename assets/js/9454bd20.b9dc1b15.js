"use strict";(self.webpackChunkmy_website=self.webpackChunkmy_website||[]).push([[4655],{3905:(e,n,t)=>{t.d(n,{Zo:()=>p,kt:()=>m});var r=t(7294);function a(e,n,t){return n in e?Object.defineProperty(e,n,{value:t,enumerable:!0,configurable:!0,writable:!0}):e[n]=t,e}function o(e,n){var t=Object.keys(e);if(Object.getOwnPropertySymbols){var r=Object.getOwnPropertySymbols(e);n&&(r=r.filter((function(n){return Object.getOwnPropertyDescriptor(e,n).enumerable}))),t.push.apply(t,r)}return t}function i(e){for(var n=1;n<arguments.length;n++){var t=null!=arguments[n]?arguments[n]:{};n%2?o(Object(t),!0).forEach((function(n){a(e,n,t[n])})):Object.getOwnPropertyDescriptors?Object.defineProperties(e,Object.getOwnPropertyDescriptors(t)):o(Object(t)).forEach((function(n){Object.defineProperty(e,n,Object.getOwnPropertyDescriptor(t,n))}))}return e}function c(e,n){if(null==e)return{};var t,r,a=function(e,n){if(null==e)return{};var t,r,a={},o=Object.keys(e);for(r=0;r<o.length;r++)t=o[r],n.indexOf(t)>=0||(a[t]=e[t]);return a}(e,n);if(Object.getOwnPropertySymbols){var o=Object.getOwnPropertySymbols(e);for(r=0;r<o.length;r++)t=o[r],n.indexOf(t)>=0||Object.prototype.propertyIsEnumerable.call(e,t)&&(a[t]=e[t])}return a}var s=r.createContext({}),l=function(e){var n=r.useContext(s),t=n;return e&&(t="function"==typeof e?e(n):i(i({},n),e)),t},p=function(e){var n=l(e.components);return r.createElement(s.Provider,{value:n},e.children)},u="mdxType",d={inlineCode:"code",wrapper:function(e){var n=e.children;return r.createElement(r.Fragment,{},n)}},f=r.forwardRef((function(e,n){var t=e.components,a=e.mdxType,o=e.originalType,s=e.parentName,p=c(e,["components","mdxType","originalType","parentName"]),u=l(t),f=a,m=u["".concat(s,".").concat(f)]||u[f]||d[f]||o;return t?r.createElement(m,i(i({ref:n},p),{},{components:t})):r.createElement(m,i({ref:n},p))}));function m(e,n){var t=arguments,a=n&&n.mdxType;if("string"==typeof e||a){var o=t.length,i=new Array(o);i[0]=f;var c={};for(var s in n)hasOwnProperty.call(n,s)&&(c[s]=n[s]);c.originalType=e,c[u]="string"==typeof e?e:a,i[1]=c;for(var l=2;l<o;l++)i[l]=t[l];return r.createElement.apply(null,i)}return r.createElement.apply(null,t)}f.displayName="MDXCreateElement"},5950:(e,n,t)=>{t.r(n),t.d(n,{assets:()=>s,contentTitle:()=>i,default:()=>d,frontMatter:()=>o,metadata:()=>c,toc:()=>l});var r=t(7462),a=(t(7294),t(3905));const o={},i="Scene",c={unversionedId:"scene",id:"version-2023.7.6/scene",title:"Scene",description:"Scenes are essentially the equivalent of a full level or a entire area of a game world.",source:"@site/versioned_docs/version-2023.7.6/scene.mdx",sourceDirName:".",slug:"/scene",permalink:"/LowZero/docs/2023.7.6/scene",draft:!1,tags:[],version:"2023.7.6",frontMatter:{},sidebar:"tutorialSidebar",previous:{title:"Region",permalink:"/LowZero/docs/2023.7.6/region"}},s={},l=[{value:"Creating scenes",id:"creating-scenes",level:2},{value:"Switching scenes",id:"switching-scenes",level:2}],p={toc:l},u="wrapper";function d(e){let{components:n,...t}=e;return(0,a.kt)(u,(0,r.Z)({},p,t,{components:n,mdxType:"MDXLayout"}),(0,a.kt)("h1",{id:"scene"},"Scene"),(0,a.kt)("p",null,"Scenes are essentially the equivalent of a full level or a entire area of a game world.\nThere can ever only one scene be loaded at a single point in time."),(0,a.kt)("p",null,"One scene is usually made up of multiple ",(0,a.kt)("a",{parentName:"p",href:"region"},"regions")," wether streamed or not.\nThese ",(0,a.kt)("a",{parentName:"p",href:"region"},"regions")," will be loaded together with their assigned scene and unloaded when the scene is changed."),(0,a.kt)("h2",{id:"creating-scenes"},"Creating scenes"),(0,a.kt)("p",null,"To create a new scene the ",(0,a.kt)("em",{parentName:"p"},"Scene")," entry in the top menu can be used.\nThe dropdown menu lists an option to create a new scene."),(0,a.kt)("h2",{id:"switching-scenes"},"Switching scenes"),(0,a.kt)("p",null,"The dropdown menu ",(0,a.kt)("em",{parentName:"p"},"Scene")," which can be found at the top of the ",(0,a.kt)("a",{parentName:"p",href:"widgets/window_layout"},"UI")," lists all created scenes.\nUsers can click of any of the listed scenes to load them.\nDoing this will unload the currently loaded scene."),(0,a.kt)("p",null,"The last loaded scene will get saved and reopened when the engine is restarted."))}d.isMDXComponent=!0}}]);