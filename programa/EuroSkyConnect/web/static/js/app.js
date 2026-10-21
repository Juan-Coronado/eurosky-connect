const API_BASE = location.origin;
    const GOOGLE_MAPS_API_KEY = window.EUROSKY_CONFIG?.googleMapsApiKey || '';
    let map;
    let marcadores = [];
    let lineas = [];
    let googleMapsListo = false;
    let aeronavesDisponibles = [];
    let aeronaveActivaCodigo = '';
    let datosOperacionActual = null;
    let datosComparacionActual = null;
    let datosMonteCarloActual = null;
    let origenCalculadoCodigo = '';
    let aeronaveCalculadaCodigo = '';
    let resultadosOperativos = {};
    let algoritmoRecomendado = '';
    const moneyEUR = new Intl.NumberFormat('es-PE', { style: 'currency', currency: 'EUR' });

    function moneyLocal(valor, moneda) {
      try {
        return new Intl.NumberFormat('es-PE', { style: 'currency', currency: moneda }).format(valor);
      } catch {
        return `${moneda} ${Number(valor).toFixed(2)}`;
      }
    }

    function mostrarError(texto) {
      const box = document.getElementById('errorBox');
      box.textContent = texto;
      box.style.display = 'block';
    }

    function limpiarError() {
      const box = document.getElementById('errorBox');
      box.textContent = '';
      box.style.display = 'none';
    }

    function sincronizarAeronaves(data) {
      aeronavesDisponibles = data.aeronaves && data.aeronaves.length
        ? data.aeronaves
        : [data.avion];
      const activas = aeronavesDisponibles;
      const activa = activas.find(a => a.activa) || data.avion || activas[0];
      if (!activa) return;
      aeronaveActivaCodigo = activa.codigo;

      document.getElementById('avModelo').value = activa.modelo;
      document.getElementById('avCapacidad').value = activa.capacidad;
      document.getElementById('avCosto').value = activa.costoDiario;
      document.getElementById('avAutonomia').value = activa.autonomiaKm || '';

      const modeloSelector = document.getElementById('modeloOperativo');
      modeloSelector.innerHTML = activas
        .map(a => `<option value="${a.codigo}">${a.codigo} - ${a.modelo} (${a.capacidad} pax)</option>`)
        .join('');
      modeloSelector.value = activa.codigo;

      const selectorGestion = document.getElementById('avSeleccionar');
      selectorGestion.innerHTML = modeloSelector.innerHTML;
      selectorGestion.value = activa.codigo;

      document.getElementById('aeronavesLista').textContent = activas
        .map(a => `${a.codigo}: ${a.modelo}, ${a.capacidad} pax, EUR ${Number(a.costoDiario).toFixed(2)}/dia${a.activa ? ' [activa]' : ''}`)
        .join(' | ');
    }

    function cargarFormularioAeronave(codigo) {
      const seleccionada = aeronavesDisponibles.find(a => a.codigo === codigo);
      if (!seleccionada) return;
      document.getElementById('avModelo').value = seleccionada.modelo;
      document.getElementById('avCapacidad').value = seleccionada.capacidad;
      document.getElementById('avCosto').value = seleccionada.costoDiario;
      document.getElementById('avAutonomia').value = seleccionada.autonomiaKm || '';
    }

    function iniciarMapa() {
      googleMapsListo = true;
      map = new google.maps.Map(document.getElementById('map'), {
        center: { lat: 46.5, lng: 4.5 },
        zoom: 4,
        mapTypeId: 'roadmap'
      });
      const claveSeleccionada = document.getElementById('algoritmoItinerario').value;
      if (resultadosOperativos[claveSeleccionada]) {
        dibujarRecorrido(resultadosOperativos[claveSeleccionada].detalle || []);
      }
    }

    function cargarGoogleMaps() {
      if (!GOOGLE_MAPS_API_KEY) {
        document.getElementById('map').innerHTML =
          '<div style="padding:18px;color:#991b1b;background:#fff1f2;border:1px solid #fecaca;border-radius:6px;">' +
          'Mapa opcional sin configurar. Define GOOGLE_MAPS_API_KEY al iniciar el servidor; los cálculos siguen disponibles.' +
          '</div>';
        mostrarError('Para usar Google Maps API debes colocar una API key válida en web/index.html.');
        cargarAeropuertos();
        return;
      }
      const script = document.createElement('script');
      script.src = `https://maps.googleapis.com/maps/api/js?key=${GOOGLE_MAPS_API_KEY}&callback=iniciarMapa&loading=async`;
      script.async = true;
      script.defer = true;
      script.onerror = () => {
        document.getElementById('map').innerHTML =
          '<div style="padding:18px;color:#991b1b;background:#fff1f2;border:1px solid #fecaca;border-radius:6px;">' +
          'No se pudo cargar Google Maps. Los cálculos y resultados continúan disponibles.' +
          '</div>';
      };
      document.head.appendChild(script);
    }

    function limpiarMapa() {
      marcadores.forEach(m => m.setMap(null));
      lineas.forEach(l => l.setMap(null));
      marcadores = [];
      lineas = [];
    }

    async function cargarAeropuertos(preservarOrigen = false, calcularInicial = false) {
      const origenActual = document.getElementById('origen').value;
      const modeloActual = document.getElementById('modeloOperativo').value;
      const data = await fetch(`${API_BASE}/api/aeropuertos`).then(r => r.json());
      sincronizarAeronaves(data);
      const select = document.getElementById('origen');
      select.innerHTML = data.aeropuertos
        .map(a => `<option value="${a.codigo}">${a.codigo} - ${a.ciudad}, ${a.pais} (${a.moneda})</option>`)
        .join('');
      select.value = preservarOrigen && origenActual ? origenActual : 'MAD';
      if (preservarOrigen && modeloActual) document.getElementById('modeloOperativo').value = modeloActual;
      if (calcularInicial) await calcularRuta();
    }

    async function aplicarModeloOperativoSeleccionado() {
      const codigo = document.getElementById('modeloOperativo').value;
      if (!codigo || codigo === aeronaveActivaCodigo) return;
      await llamarGestion('/api/aeronaves/seleccionar', { codigo }, false);
      aeronaveActivaCodigo = codigo;
    }

    async function calcularRuta() {
      try {
        limpiarError();
        await aplicarModeloOperativoSeleccionado();
        const origen = document.getElementById('origen').value;
        const data = await fetch(`${API_BASE}/api/rutas/calcular?origen=${origen}`).then(r => r.json());
        datosOperacionActual = data;
        aeronaveActivaCodigo = data.aeronaveCodigo || aeronaveActivaCodigo;
        origenCalculadoCodigo = origen;
        aeronaveCalculadaCodigo = aeronaveActivaCodigo;
        datosComparacionActual = null;
        datosMonteCarloActual = null;
        resultadosOperativos = {};

        document.getElementById('capacidad').textContent = `${data.capacidadAeronave || data.capacidadMaxima} pasajeros`;
        document.getElementById('alquiler').textContent = moneyEUR.format(data.costoAlquilerEUR);
        document.getElementById('beneficio').textContent = moneyEUR.format(data.beneficioNetoEUR);
        document.getElementById('tiempo').textContent = `${data.tiempoTotalMin} min`;

        const tabla = document.getElementById('tabla');
        if (!data.ruta.length) {
          tabla.innerHTML = '<div class="text-secondary">No se encontró una ruta rentable.</div>';
          if (googleMapsListo) limpiarMapa();
        } else {
          tabla.innerHTML = data.ruta.map(r => `
            <article class="tramo-card">
              <div class="tramo-encabezado">
                <div><strong>${r.origen} → ${r.destino}</strong><div class="tramo-ciudades">${r.origenCiudad} a ${r.destinoCiudad}</div></div>
                <strong>${r.tiempoMin} min</strong>
              </div>
              <div class="tramo-datos">
                <div class="tramo-dato">
                  <span class="dato-label">Costo</span>
                  <span class="dato-principal">${moneyEUR.format(r.costoEUR)}</span>
                </div>
                <div class="tramo-dato">
                  <span class="dato-label">Beneficio</span>
                  <span class="dato-principal positivo">${moneyEUR.format(r.beneficioEUR)}</span>
                </div>
              </div>
            </article>
          `).join('');

          if (googleMapsListo) {
            limpiarMapa();
            const bounds = new google.maps.LatLngBounds();
            data.ruta.forEach(r => {
              const a = { lat: r.lat1, lng: r.lng1 };
              const b = { lat: r.lat2, lng: r.lng2 };
              bounds.extend(a);
              bounds.extend(b);
              marcadores.push(new google.maps.Marker({ position: a, map, title: `${r.origen} - ${r.origenCiudad}` }));
              marcadores.push(new google.maps.Marker({ position: b, map, title: `${r.destino} - ${r.destinoCiudad}` }));
              lineas.push(new google.maps.Polyline({
                path: [a, b],
                geodesic: true,
                strokeColor: '#dc2626',
                strokeOpacity: 0.95,
                strokeWeight: 4,
                map
              }));
            });
            map.fitBounds(bounds);
          }
        }
        await cargarApartadosAnalisis(origen);
      } catch (err) {
        mostrarError('No se pudo conectar con el backend. Primero ejecute INICIAR_SISTEMA.bat. Detalle: ' + err.message);
      }
    }

    async function simularMonteCarlo(origen) {
      document.getElementById('mc').textContent = 'Ejecutando simulación...';
      try {
        const data = await fetch(`${API_BASE}/api/simulacion/montecarlo?origen=${origen}&iteraciones=1000`).then(r => r.json());
        datosMonteCarloActual = data;
        document.getElementById('mc').innerHTML =
          `<strong>1000 iteraciones</strong>
          <div class="montecarlo-metricas">
            <div class="montecarlo-metrica"><span class="dato-label">Promedio</span><span class="dato-principal">${moneyEUR.format(data.beneficioPromedio)}</span></div>
            <div class="montecarlo-metrica"><span class="dato-label">Mínimo</span><span class="dato-principal">${moneyEUR.format(data.beneficioMinimo)}</span></div>
            <div class="montecarlo-metrica"><span class="dato-label">Máximo</span><span class="dato-principal">${moneyEUR.format(data.beneficioMaximo)}</span></div>
          </div>`;
      } catch (err) {
        document.getElementById('mc').textContent = 'No se pudo ejecutar Monte Carlo: ' + err.message;
      }
    }

    function filaAlgoritmo(nombre, tipo, objetivo, resultado, tiempo = '-', costo = '-', ingreso = '-', recomendado = false) {
      return `
        <article class="algoritmo-fila ${recomendado ? 'recomendado' : ''}">
          <div class="algoritmo-nombre">
            <strong>${nombre}</strong>
            <span class="algoritmo-tipo">${tipo}</span>
            ${recomendado ? '<span class="etiqueta-recomendada">Mejor resultado</span>' : ''}
          </div>
          <div class="algoritmo-objetivo">${objetivo}</div>
          <div class="algoritmo-resultado">${resultado}</div>
          <div class="algoritmo-valores">
            <div class="algoritmo-valor"><span>Tiempo</span><strong>${tiempo}</strong></div>
            <div class="algoritmo-valor"><span>Costo</span><strong>${costo}</strong></div>
            <div class="algoritmo-valor"><span>Ingreso</span><strong>${ingreso}</strong></div>
          </div>
        </article>`;
    }

    const nombresAlgoritmos = {
      greedy: 'Greedy',
      dijkstra: 'Dijkstra + heap',
      programacionDinamica: 'Programación dinámica'
    };

    function dibujarRecorrido(detalle) {
      if (!googleMapsListo) return;
      limpiarMapa();
      if (!detalle.length) return;
      const bounds = new google.maps.LatLngBounds();
      detalle.forEach(r => {
        const a = { lat: r.lat1, lng: r.lng1 };
        const b = { lat: r.lat2, lng: r.lng2 };
        bounds.extend(a);
        bounds.extend(b);
        marcadores.push(new google.maps.Marker({ position: a, map, title: `${r.origen} - ${r.origenCiudad}` }));
        marcadores.push(new google.maps.Marker({ position: b, map, title: `${r.destino} - ${r.destinoCiudad}` }));
        lineas.push(new google.maps.Polyline({
          path: [a, b], geodesic: true, strokeColor: '#dc2626',
          strokeOpacity: 0.95, strokeWeight: 4, map
        }));
      });
      map.fitBounds(bounds);
    }

    function mostrarResultadoAlgoritmo(clave) {
      const resultado = resultadosOperativos[clave];
      if (!resultado) return;
      const esRecomendado = clave === algoritmoRecomendado;
      const detalle = resultado.detalle || [];

      document.getElementById('capacidad').textContent = `${datosOperacionActual.capacidadAeronave} pasajeros`;
      document.getElementById('alquiler').textContent = moneyEUR.format(datosOperacionActual.costoAlquilerEUR);
      document.getElementById('tiempo').textContent = `${resultado.tiempoMin} min`;
      document.getElementById('beneficio').textContent = moneyEUR.format(resultado.beneficioNetoEUR);
      document.getElementById('tituloItinerario').textContent = esRecomendado
        ? 'Itinerario recomendado'
        : 'Itinerario seleccionado';
      document.getElementById('volverRecomendado').hidden = esRecomendado;

      const tabla = document.getElementById('tabla');
      tabla.innerHTML = detalle.length ? detalle.map(r => `
        <article class="tramo-card">
          <div class="tramo-encabezado">
            <div><strong>${r.origen} → ${r.destino}</strong><div class="tramo-ciudades">${r.origenCiudad} a ${r.destinoCiudad}</div></div>
            <strong>${r.tiempoMin} min</strong>
          </div>
          <div class="tramo-datos">
            <div class="tramo-dato">
              <span class="dato-label">Costo</span>
              <span class="dato-principal">${moneyEUR.format(r.costoEUR)}</span>
            </div>
            <div class="tramo-dato">
              <span class="dato-label">Beneficio</span>
              <span class="dato-principal positivo">${moneyEUR.format(r.beneficioEUR)}</span>
            </div>
          </div>
        </article>
      `).join('') : '<div class="text-secondary">Este algoritmo no encontró tramos para el destino de comparación.</div>';
      dibujarRecorrido(detalle);
    }

    function configurarItinerarios(data) {
      resultadosOperativos = {
        greedy: data.greedy,
        dijkstra: data.dijkstra,
        programacionDinamica: data.programacionDinamica
      };
      algoritmoRecomendado = Object.entries(resultadosOperativos)
        .sort((a, b) => b[1].beneficioNetoEUR - a[1].beneficioNetoEUR)[0][0];

      const selector = document.getElementById('algoritmoItinerario');
      selector.innerHTML = Object.keys(resultadosOperativos)
        .map(clave => `<option value="${clave}">${nombresAlgoritmos[clave]}${clave === algoritmoRecomendado ? ' - recomendado' : ''}</option>`)
        .join('');
      selector.disabled = false;
      selector.value = algoritmoRecomendado;
      mostrarResultadoAlgoritmo(algoritmoRecomendado);
    }

    async function compararAlgoritmos(origen) {
      document.getElementById('analisis').innerHTML = '<div class="text-secondary">Analizando red...</div>';
      document.getElementById('comparacion').innerHTML = '<div class="text-secondary">Comparando rutas...</div>';
      try {
        const data = await fetch(`${API_BASE}/api/algoritmos/comparar?origen=${origen}`).then(r => r.json());
        datosComparacionActual = data;
        configurarItinerarios(data);

        const negativas = data.bellmanFord.rutasNegativas || [];
        const origenTexto = document.getElementById('origen').selectedOptions[0]?.textContent || origen;
        const origenDetalle = origenTexto.replace(/^[A-Z]{3}\s+-\s+/, '').replace(/\s+\([A-Z]{3}\)$/, '');
        document.getElementById('analisis').innerHTML = `
          <div class="analisis-grid">
            <article class="analisis-item">
              <h3>BFS - recorrido por niveles</h3>
              <p>Finalidad: validar cobertura desde el origen. Recorre primero los aeropuertos más cercanos en cantidad de saltos.</p>
              <p><strong>${data.bfs.visitados} aeropuertos alcanzables desde ${origenDetalle}.</strong></p>
              <div class="ruta-codigos">${data.bfs.ordenCompleto}</div>
            </article>
            <article class="analisis-item">
              <h3>DFS - recorrido en profundidad</h3>
              <p>Finalidad: comprobar exploración completa siguiendo cada rama hasta el final antes de retroceder.</p>
              <p><strong>${data.dfs.visitados} aeropuertos explorados desde ${origenDetalle}.</strong></p>
              <div class="ruta-codigos">${data.dfs.ordenCompleto}</div>
            </article>
            <article class="analisis-item alerta">
              <h3>Bellman-Ford - pesos negativos</h3>
              <p>Finalidad: analizar rutas con peso neto costo menos ingreso. Un peso negativo indica que el ingreso esperado supera el costo operativo del tramo.</p>
              <div class="analisis-resumen">
                Se identificaron <strong>${negativas.length}</strong> rutas con peso neto negativo dentro de la zona alcanzable.
                ${data.bellmanFord.cicloNegativo
                  ? 'El algoritmo detectó un ciclo negativo, señal de recorridos donde la rentabilidad acumulada puede crecer si no se aplican restricciones operativas.'
                  : 'No se detectó ciclo negativo para el destino de comparación.'}
              </div>
            </article>
          </div>`;

        document.getElementById('comparacion').innerHTML = `
          <div class="algoritmo-cabecera">
            <span>Algoritmo</span><span>Qué analiza</span><span>Resultado obtenido</span><span>Métricas</span>
          </div>` +
          filaAlgoritmo(
            'Greedy', 'Selección rápida',
            'Elige en cada paso el vuelo con mayor beneficio por minuto.',
            `Ruta seleccionada: ${data.greedy.ruta}`,
            `${data.greedy.tiempoMin} min`, moneyEUR.format(data.greedy.costoTotalEUR), moneyEUR.format(data.greedy.ingresoEUR), algoritmoRecomendado === 'greedy'
          ) +
          filaAlgoritmo(
            'Dijkstra + heap', 'Menor costo',
            `Busca la ruta de menor costo desde el origen hasta ${data.destinoComparacion}.`,
            `Ruta de menor costo: ${data.dijkstra.ruta}`,
            `${data.dijkstra.tiempoMin} min`, moneyEUR.format(data.dijkstra.costoTotalEUR), moneyEUR.format(data.dijkstra.ingresoEUR), algoritmoRecomendado === 'dijkstra'
          ) +
          filaAlgoritmo(
            'Programación dinámica', 'Máximo beneficio',
            'Evalúa combinaciones posibles respetando 480 minutos y máximo tres tramos.',
            `Mejor ruta encontrada: ${data.programacionDinamica.ruta}`,
            `${data.programacionDinamica.tiempoMin} min`, moneyEUR.format(data.programacionDinamica.costoTotalEUR), moneyEUR.format(data.programacionDinamica.ingresoEUR), algoritmoRecomendado === 'programacionDinamica'
          );
      } catch (err) {
        document.getElementById('analisis').innerHTML = '<div class="text-secondary">No se pudo ejecutar el análisis.</div>';
        document.getElementById('comparacion').innerHTML = '<div class="text-secondary">No se pudo comparar los algoritmos.</div>';
      }
    }

    async function cargarApartadosAnalisis(origen) {
      await Promise.all([
        simularMonteCarlo(origen),
        compararAlgoritmos(origen)
      ]);
    }

    function importePDF(valor) {
      if (valor === null || valor === undefined || valor === '-') return '-';
      return `EUR ${Number(valor).toLocaleString('en-US', { minimumFractionDigits: 2, maximumFractionDigits: 2 })}`;
    }

    function asegurarEspacioPDF(doc, posicionY, espacioNecesario = 35) {
      if (posicionY + espacioNecesario <= 282) return posicionY;
      doc.addPage();
      return 18;
    }

    function dibujarGraficoComparativoPDF(doc, resultados, recomendado, posicionY) {
      const claves = ['greedy', 'dijkstra', 'programacionDinamica'];
      const colores = {
        greedy: [48, 113, 173],
        dijkstra: [168, 121, 0],
        programacionDinamica: [19, 118, 111]
      };
      const series = [
        { etiqueta: 'Costo total', campo: 'costoTotalEUR' },
        { etiqueta: 'Ingreso', campo: 'ingresoEUR' },
        { etiqueta: 'Beneficio neto', campo: 'beneficioNetoEUR' }
      ];
      const maximo = Math.max(...series.flatMap(serie => claves.map(clave => Number(resultados[clave][serie.campo]) || 0)), 1);
      const baseY = posicionY + 68;
      const altoGrafico = 38;
      const centros = [48, 105, 162];
      const anchoBarra = 8;
      const separacion = 2;

      doc.setTextColor(19, 41, 61);
      doc.setFont('helvetica', 'bold');
      doc.setFontSize(11.5);
      doc.text('Comparación financiera de algoritmos', 14, posicionY);
      doc.setFont('helvetica', 'normal');
      doc.setFontSize(7.5);
      doc.setTextColor(91, 108, 124);
      doc.text('Costo total, ingreso y beneficio neto expresados en miles de EUR.', 14, posicionY + 5.5);

      claves.forEach((clave, indice) => {
        const leyendaX = 20 + indice * 58;
        doc.setFillColor(...colores[clave]);
        doc.rect(leyendaX, posicionY + 10, 4, 4, 'F');
        doc.setTextColor(19, 41, 61);
        doc.setFont('helvetica', clave === recomendado ? 'bold' : 'normal');
        doc.setFontSize(7.2);
        doc.text(`${nombresAlgoritmos[clave]}${clave === recomendado ? ' (recomendado)' : ''}`, leyendaX + 6, posicionY + 13.3);
      });

      doc.setDrawColor(153, 169, 181);
      doc.setLineWidth(0.3);
      doc.line(18, baseY, 192, baseY);
      series.forEach((serie, serieIndice) => {
        claves.forEach((clave, algoritmoIndice) => {
          const valor = Number(resultados[clave][serie.campo]) || 0;
          const altoBarra = Math.max(0, (valor / maximo) * altoGrafico);
          const barraX = centros[serieIndice] + (algoritmoIndice - 1) * (anchoBarra + separacion) - anchoBarra / 2;
          doc.setFillColor(...colores[clave]);
          doc.roundedRect(barraX, baseY - altoBarra, anchoBarra, altoBarra, 0.8, 0.8, 'F');
          if (clave === recomendado) {
            doc.setDrawColor(19, 41, 61);
            doc.setLineWidth(0.45);
            doc.roundedRect(barraX - 0.6, baseY - altoBarra - 0.6, anchoBarra + 1.2, altoBarra + 1.2, 1, 1, 'S');
          }
          doc.setTextColor(19, 41, 61);
          doc.setFont('helvetica', 'bold');
          doc.setFontSize(6.2);
          doc.text(`${(valor / 1000).toFixed(1)}k`, barraX + anchoBarra / 2, baseY - altoBarra - 2, { align: 'center' });
        });
        doc.setTextColor(45, 61, 76);
        doc.setFont('helvetica', 'normal');
        doc.setFontSize(7.5);
        doc.text(serie.etiqueta, centros[serieIndice], baseY + 5.5, { align: 'center' });
      });

      return posicionY + 79;
    }

    async function descargarReporte() {
      limpiarError();
      if (!datosOperacionActual || !datosComparacionActual || !Object.keys(resultadosOperativos).length) {
        mostrarError('Primero calcule una ruta para generar el reporte PDF.');
        return;
      }
      if (!window.jspdf || !window.jspdf.jsPDF) {
        mostrarError('No se pudo cargar el generador PDF. Verifique la conexión a internet y vuelva a intentarlo.');
        return;
      }

      try {
        const { jsPDF } = window.jspdf;
        const doc = new jsPDF({ orientation: 'portrait', unit: 'mm', format: 'a4' });
        if (typeof doc.autoTable !== 'function') throw new Error('El módulo de tablas PDF no está disponible.');

        const origen = origenCalculadoCodigo;
        const opcionOrigen = Array.from(document.getElementById('origen').options)
          .find(opcion => opcion.value === origen);
        const origenTexto = opcionOrigen?.textContent || origen;
        const codigoAeronave = aeronaveCalculadaCodigo;
        const aeronave = aeronavesDisponibles.find(a => a.codigo === codigoAeronave);
        const claves = ['greedy', 'dijkstra', 'programacionDinamica'];
        const navy = [19, 41, 61];
        const teal = [19, 118, 111];
        const lightTeal = [231, 245, 242];
        const lightGray = [241, 244, 246];

        doc.setFillColor(...navy);
        doc.rect(0, 0, 210, 30, 'F');
        doc.setTextColor(255, 255, 255);
        doc.setFont('helvetica', 'bold');
        doc.setFontSize(19);
        doc.text('EuroSky Connect', 14, 14);
        doc.setFontSize(10);
        doc.setFont('helvetica', 'normal');
        doc.text('Reporte de planificación y comparación de rutas', 14, 22);

        doc.setTextColor(...navy);
        doc.setFontSize(11);
        doc.setFont('helvetica', 'bold');
        doc.text('Escenario operativo', 14, 40);
        doc.setFont('helvetica', 'normal');
        doc.setFontSize(9.5);
        doc.text(`Origen: ${origenTexto}`, 14, 47);
        doc.text(`Aeronave: ${aeronave ? `${aeronave.codigo} - ${aeronave.modelo}` : codigoAeronave}`, 14, 53);
        doc.text(`Capacidad: ${datosOperacionActual.capacidadAeronave} pasajeros`, 14, 59);
        doc.text(`Alquiler diario: ${importePDF(datosOperacionActual.costoAlquilerEUR)}`, 105, 59);
        doc.text(`Fecha de generación: ${new Date().toLocaleString('es-PE')}`, 105, 47);
        doc.text('Restricciones: jornada máxima 480 min, hasta 3 tramos y escala de 45 min por tramo.', 14, 65);

        doc.setFont('helvetica', 'bold');
        doc.setFontSize(12);
        doc.text('Resumen comparativo', 14, 76);
        const resumen = claves.map(clave => {
          const r = resultadosOperativos[clave];
          return [
            `${nombresAlgoritmos[clave]}${clave === algoritmoRecomendado ? ' - RECOMENDADO' : ''}`,
            r.ruta,
            `${r.tiempoMin} min`,
            importePDF(r.costoTotalEUR),
            importePDF(r.ingresoEUR),
            importePDF(r.beneficioNetoEUR)
          ];
        });
        doc.autoTable({
          startY: 80,
          head: [['Algoritmo', 'Ruta', 'Tiempo', 'Costo total', 'Ingreso', 'Beneficio neto']],
          body: resumen,
          theme: 'grid',
          styles: { font: 'helvetica', fontSize: 7.4, cellPadding: 2.2, textColor: navy },
          headStyles: { fillColor: navy, textColor: 255, fontStyle: 'bold' },
          columnStyles: { 0: { cellWidth: 28 }, 1: { cellWidth: 47 }, 2: { cellWidth: 16 }, 3: { cellWidth: 28 }, 4: { cellWidth: 28 }, 5: { cellWidth: 35 } },
          didParseCell: data => {
            if (data.section === 'body' && claves[data.row.index] === algoritmoRecomendado) {
              data.cell.styles.fillColor = lightTeal;
              data.cell.styles.fontStyle = 'bold';
            }
          }
        });

        let y = doc.lastAutoTable.finalY + 11;
        claves.forEach(clave => {
          const resultado = resultadosOperativos[clave];
          const recomendado = clave === algoritmoRecomendado;
          y = asegurarEspacioPDF(doc, y, 48);

          if (recomendado) {
            doc.setFillColor(...lightTeal);
            doc.roundedRect(14, y - 5, 182, 10, 1.5, 1.5, 'F');
          }
          doc.setTextColor(...navy);
          doc.setFont('helvetica', 'bold');
          doc.setFontSize(11.5);
          doc.text(nombresAlgoritmos[clave], 14, y);
          if (recomendado) {
            doc.setTextColor(...teal);
            doc.setFontSize(8.5);
            doc.text('ALGORITMO RECOMENDADO', 148, y);
          }
          y += 5;

          const filas = (resultado.detalle || []).map(tramo => [
            `${tramo.origen}-${tramo.destino}`,
            `${tramo.tiempoMin + 45} min`,
            String(tramo.pasajeros),
            importePDF(tramo.costoEUR),
            importePDF(tramo.ingresoEUR),
            importePDF(tramo.beneficioEUR)
          ]);
          filas.push(
            ['Subtotal de rutas', `${resultado.tiempoMin} min`, '-', importePDF(resultado.costoEUR), importePDF(resultado.ingresoEUR), importePDF(resultado.beneficioEUR)],
            [`Alquiler diario ${codigoAeronave}`, '-', '-', importePDF(datosOperacionActual.costoAlquilerEUR), '-', importePDF(-datosOperacionActual.costoAlquilerEUR)],
            ['Total de jornada', `${resultado.tiempoMin} min`, '-', importePDF(resultado.costoTotalEUR), importePDF(resultado.ingresoEUR), importePDF(resultado.beneficioNetoEUR)]
          );

          doc.autoTable({
            startY: y,
            head: [['Tramo', 'Tiempo + escala', 'Pasajeros', 'Costo EUR', 'Ingreso EUR', 'Beneficio EUR']],
            body: filas,
            theme: 'grid',
            styles: { font: 'helvetica', fontSize: 7.8, cellPadding: 2.1, textColor: navy },
            headStyles: { fillColor: recomendado ? teal : navy, textColor: 255, fontStyle: 'bold' },
            columnStyles: { 0: { cellWidth: 36 }, 1: { cellWidth: 25 }, 2: { cellWidth: 20, halign: 'center' }, 3: { cellWidth: 32, halign: 'right' }, 4: { cellWidth: 32, halign: 'right' }, 5: { cellWidth: 34, halign: 'right' } },
            didParseCell: data => {
              if (data.section !== 'body') return;
              const etiqueta = data.row.raw[0];
              if (etiqueta === 'Subtotal de rutas') data.cell.styles.fillColor = lightGray;
              if (etiqueta === 'Total de jornada') {
                data.cell.styles.fillColor = recomendado ? lightTeal : lightGray;
                data.cell.styles.fontStyle = 'bold';
              }
            }
          });
          y = doc.lastAutoTable.finalY + 11;
        });

        y = asegurarEspacioPDF(doc, y, 88);
        y = dibujarGraficoComparativoPDF(doc, resultadosOperativos, algoritmoRecomendado, y);

        y = asegurarEspacioPDF(doc, y, 48);
        doc.setTextColor(...navy);
        doc.setFont('helvetica', 'bold');
        doc.setFontSize(11.5);
        doc.text('Simulación Monte Carlo - ruta Greedy', 14, y);
        if (datosMonteCarloActual) {
          doc.autoTable({
            startY: y + 4,
            head: [['Iteraciones', 'Beneficio promedio', 'Beneficio mínimo', 'Beneficio máximo']],
            body: [[
              String(datosMonteCarloActual.iteraciones),
              importePDF(datosMonteCarloActual.beneficioPromedio),
              importePDF(datosMonteCarloActual.beneficioMinimo),
              importePDF(datosMonteCarloActual.beneficioMaximo)
            ]],
            theme: 'grid',
            styles: { font: 'helvetica', fontSize: 8.5, cellPadding: 2.4, textColor: navy, halign: 'center' },
            headStyles: { fillColor: teal, textColor: 255, fontStyle: 'bold' }
          });
          y = doc.lastAutoTable.finalY + 10;
        } else {
          doc.setFont('helvetica', 'normal');
          doc.setFontSize(9);
          doc.text('No se dispone de resultados Monte Carlo para este cálculo.', 14, y + 7);
          y += 14;
        }

        y = asegurarEspacioPDF(doc, y, 40);
        doc.setFont('helvetica', 'bold');
        doc.setFontSize(11.5);
        doc.text('Análisis de la red', 14, y);
        const negativas = datosComparacionActual.bellmanFord.rutasNegativas || [];
        doc.autoTable({
          startY: y + 4,
          head: [['Método', 'Resultado', 'Finalidad']],
          body: [
            ['BFS', `${datosComparacionActual.bfs.visitados} aeropuertos alcanzables`, 'Cobertura por niveles desde el origen.'],
            ['DFS', `${datosComparacionActual.dfs.visitados} aeropuertos explorados`, 'Exploración en profundidad de la red.'],
            ['Bellman-Ford', `${negativas.length} pesos negativos; ciclo: ${datosComparacionActual.bellmanFord.cicloNegativo ? 'sí' : 'no'}`, 'Análisis de costo menos ingreso.']
          ],
          theme: 'grid',
          styles: { font: 'helvetica', fontSize: 8.2, cellPadding: 2.4, textColor: navy },
          headStyles: { fillColor: navy, textColor: 255, fontStyle: 'bold' },
          columnStyles: { 0: { cellWidth: 32 }, 1: { cellWidth: 55 }, 2: { cellWidth: 92 } }
        });

        const paginas = doc.getNumberOfPages();
        for (let pagina = 1; pagina <= paginas; pagina++) {
          doc.setPage(pagina);
          doc.setDrawColor(210, 220, 228);
          doc.line(14, 288, 196, 288);
          doc.setTextColor(101, 117, 134);
          doc.setFont('helvetica', 'normal');
          doc.setFontSize(7.5);
          doc.text('EuroSky Connect - Elaboración propia a partir de los parámetros del prototipo.', 14, 293);
          doc.text(`Página ${pagina} de ${paginas}`, 196, 293, { align: 'right' });
        }

        doc.save(`reporte_${origen}_${codigoAeronave}.pdf`);
      } catch (err) {
        mostrarError('No se pudo generar el reporte PDF: ' + err.message);
      }
    }

    async function llamarGestion(endpoint, parametros, mostrarMensaje = true) {
      const respuesta = await fetch(`${API_BASE}${endpoint}?${new URLSearchParams(parametros)}`, { method: "POST", headers: { "X-EuroSky-Request": "local" } });
      const data = await respuesta.json();
      if (!respuesta.ok) throw new Error(data.error || 'No se pudo completar la operación.');
      if (mostrarMensaje) alert(data.mensaje);
      return data;
    }

    async function registrarAeropuerto() {
      try {
        await llamarGestion('/api/aeropuertos/registrar', {
          codigo: aCodigo.value.toUpperCase(), nombre: aNombre.value, ciudad: aCiudad.value, pais: aPais.value,
          lat: aLat.value, lng: aLng.value, moneda: aMoneda.value.toUpperCase(), simbolo: aSimbolo.value, cambio: aCambio.value
        });
        await cargarAeropuertos(true);
      } catch (err) { mostrarError(err.message); }
    }

    async function registrarRuta() {
      try {
        await llamarGestion('/api/rutas/registrar', {
          origen: rOrigen.value.toUpperCase(), destino: rDestino.value.toUpperCase(), distancia: rDistancia.value,
          tiempo: rTiempo.value, demanda: rDemanda.value, precio: rPrecio.value, tasa: rTasa.value, costoKm: rCostoKm.value
        });
      } catch (err) { mostrarError(err.message); }
    }

    async function actualizarAeronave() {
      try {
        await llamarGestion('/api/aeronave/actualizar', {
          codigo: avSeleccionar.value,
          modelo: avModelo.value, capacidad: avCapacidad.value, costoDiario: avCosto.value, autonomia: avAutonomia.value
        });
        await cargarAeropuertos(true);
      } catch (err) { mostrarError(err.message); }
    }

    async function registrarAeronave() {
      try {
        await llamarGestion('/api/aeronaves/registrar', {
          codigo: avNuevoCodigo.value.toUpperCase(), modelo: avNuevoModelo.value, capacidad: avNuevoCapacidad.value,
          costoDiario: avNuevoCosto.value, autonomia: avNuevoAutonomia.value
        });
        await cargarAeropuertos(true);
      } catch (err) { mostrarError(err.message); }
    }

    document.getElementById('calcular').addEventListener('click', calcularRuta);
    document.getElementById('algoritmoItinerario').addEventListener('change', e => mostrarResultadoAlgoritmo(e.target.value));
    document.getElementById('volverRecomendado').addEventListener('click', () => {
      const selector = document.getElementById('algoritmoItinerario');
      selector.value = algoritmoRecomendado;
      mostrarResultadoAlgoritmo(algoritmoRecomendado);
    });
    document.getElementById('avSeleccionar').addEventListener('change', e => cargarFormularioAeronave(e.target.value));
    document.getElementById('reporte').addEventListener('click', descargarReporte);
    document.getElementById('registrarAeropuerto').addEventListener('click', registrarAeropuerto);
    document.getElementById('registrarRuta').addEventListener('click', registrarRuta);
    document.getElementById('actualizarAeronave').addEventListener('click', actualizarAeronave);
    document.getElementById('registrarAeronave').addEventListener('click', registrarAeronave);
    cargarAeropuertos(false, true);
    cargarGoogleMaps();
