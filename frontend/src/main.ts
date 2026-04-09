import { createApp } from 'vue'
import './styles/base.css'
import './styles/theme.css'
import App from './App.vue'
import { smoothWheelDirective } from './directives/smoothWheel'
import router from './router'
import { appPinia } from './stores/pinia'

const app = createApp(App)

app.directive('smooth-wheel', smoothWheelDirective)

app
  .use(appPinia)
  .use(router)
  .mount('#app')
