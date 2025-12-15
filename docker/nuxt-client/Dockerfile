# Usa una imagen base de Node.js
FROM node:22

# Instala Git
RUN apt-get update && apt-get install -y git

# Establece el directorio de trabajo
WORKDIR /usr/src/app
ARG NODE_ENV=production
ENV NODE_ENV=${NODE_ENV}

# Clona el repositorio
#RUN git clone https://github.com/Wagtial/zooproject-nuxt-client.git .
COPY ./zooproject-nuxt-client/ ./
# Copia los archivos de tu proyecto
COPY package*.json ./

# Copia los archivos de tu proyecto

COPY ./docker/nuxt-client/env_nuxt.sample .env

# Instala las dependencias
RUN npm install --omit=dev

# Construye el proyecto
RUN pwd ; ls -ailtr ; npm run build

# Expone el puerto de la aplicación
EXPOSE 3000

# Comando para iniciar el servidor en producción
# CMD ["node", ".output/server/index.mjs"]
CMD ["npm", "run", "start"]