"use strict";(self.webpackChunkmy_website=self.webpackChunkmy_website||[]).push([[5115],{3905:(e,t,r)=>{r.d(t,{Zo:()=>d,kt:()=>u});var n=r(7294);function o(e,t,r){return t in e?Object.defineProperty(e,t,{value:r,enumerable:!0,configurable:!0,writable:!0}):e[t]=r,e}function a(e,t){var r=Object.keys(e);if(Object.getOwnPropertySymbols){var n=Object.getOwnPropertySymbols(e);t&&(n=n.filter((function(t){return Object.getOwnPropertyDescriptor(e,t).enumerable}))),r.push.apply(r,n)}return r}function s(e){for(var t=1;t<arguments.length;t++){var r=null!=arguments[t]?arguments[t]:{};t%2?a(Object(r),!0).forEach((function(t){o(e,t,r[t])})):Object.getOwnPropertyDescriptors?Object.defineProperties(e,Object.getOwnPropertyDescriptors(r)):a(Object(r)).forEach((function(t){Object.defineProperty(e,t,Object.getOwnPropertyDescriptor(r,t))}))}return e}function i(e,t){if(null==e)return{};var r,n,o=function(e,t){if(null==e)return{};var r,n,o={},a=Object.keys(e);for(n=0;n<a.length;n++)r=a[n],t.indexOf(r)>=0||(o[r]=e[r]);return o}(e,t);if(Object.getOwnPropertySymbols){var a=Object.getOwnPropertySymbols(e);for(n=0;n<a.length;n++)r=a[n],t.indexOf(r)>=0||Object.prototype.propertyIsEnumerable.call(e,r)&&(o[r]=e[r])}return o}var l=n.createContext({}),c=function(e){var t=n.useContext(l),r=t;return e&&(r="function"==typeof e?e(t):s(s({},t),e)),r},d=function(e){var t=c(e.components);return n.createElement(l.Provider,{value:t},e.children)},p="mdxType",g={inlineCode:"code",wrapper:function(e){var t=e.children;return n.createElement(n.Fragment,{},t)}},m=n.forwardRef((function(e,t){var r=e.components,o=e.mdxType,a=e.originalType,l=e.parentName,d=i(e,["components","mdxType","originalType","parentName"]),p=c(r),m=o,u=p["".concat(l,".").concat(m)]||p[m]||g[m]||a;return r?n.createElement(u,s(s({ref:t},d),{},{components:r})):n.createElement(u,s({ref:t},d))}));function u(e,t){var r=arguments,o=t&&t.mdxType;if("string"==typeof e||o){var a=r.length,s=new Array(a);s[0]=m;var i={};for(var l in t)hasOwnProperty.call(t,l)&&(i[l]=t[l]);i.originalType=e,i[p]="string"==typeof e?e:o,s[1]=i;for(var c=2;c<a;c++)s[c]=r[c];return n.createElement.apply(null,s)}return n.createElement.apply(null,r)}m.displayName="MDXCreateElement"},384:(e,t,r)=>{r.r(t),r.d(t,{assets:()=>l,contentTitle:()=>s,default:()=>g,frontMatter:()=>a,metadata:()=>i,toc:()=>c});var n=r(7462),o=(r(7294),r(3905));const a={},s="Log",i={unversionedId:"widgets/logwidget",id:"version-2023.8.1/widgets/logwidget",title:"Log",description:"The log widget displays current events and messages from the engine that can be used for debugging or just as a means of communication.",source:"@site/versioned_docs/version-2023.8.1/widgets/logwidget.mdx",sourceDirName:"widgets",slug:"/widgets/logwidget",permalink:"/LowZero/docs/widgets/logwidget",draft:!1,tags:[],version:"2023.8.1",frontMatter:{},sidebar:"tutorialSidebar",previous:{title:"History",permalink:"/LowZero/docs/widgets/historywidget"},next:{title:"Profiler",permalink:"/LowZero/docs/widgets/profilerwidget"}},l={},c=[{value:"Messages",id:"messages",level:2}],d={toc:c},p="wrapper";function g(e){let{components:t,...a}=e;return(0,o.kt)(p,(0,n.Z)({},d,a,{components:t,mdxType:"MDXLayout"}),(0,o.kt)("h1",{id:"log"},"Log"),(0,o.kt)("p",null,"The log widget displays current events and messages from the engine that can be used for debugging or just as a means of communication.\nThe messages displayed in this widget are a subset of the messages logged by the engine."),(0,o.kt)("p",null,(0,o.kt)("img",{alt:"Logwidget",src:r(5010).Z,width:"714",height:"378"})),(0,o.kt)("h2",{id:"messages"},"Messages"),(0,o.kt)("p",null,"Each message displayed in the widget has a type. This type can either be ",(0,o.kt)("em",{parentName:"p"},"debug"),", ",(0,o.kt)("em",{parentName:"p"},"info"),", ",(0,o.kt)("em",{parentName:"p"},"warn"),", ",(0,o.kt)("em",{parentName:"p"},"error")," or ",(0,o.kt)("em",{parentName:"p"},"profile"),".\nThe type of the missage is displayed using the colored icon.\nBesides that each message has a content and also displays the source of the message below the content."),(0,o.kt)("admonition",{type:"note"},(0,o.kt)("p",{parentName:"admonition"},"The logwidget has a maximum amount of messages that can be displayed at a time.\nIf the buffer is full and a new message is posted, the oldest message gets removed from the widget to make space for the new one.")))}g.isMDXComponent=!0},5010:(e,t,r)=>{r.d(t,{Z:()=>n});const n=r.p+"assets/images/logwidget-1112d4bda43caa8810741b11f3bd4854.PNG"}}]);