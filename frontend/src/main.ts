import { createApp } from 'vue'
import './styles/base.css'
import './styles/theme.css'
import App from './App.vue'
import router from './router'
import { appPinia } from './stores/pinia'

createApp(App)
  .use(appPinia)
  .use(router)
  .mount('#app')
