# Builds the odelab_py extension and runs the FastAPI server + frontend
# from a single container. Render/Railway/Fly.io can all build this
# directly from the repo with no extra config beyond pointing at it.

FROM python:3.11-slim

# build-essential gives us g++ to compile the pybind11 extension.
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . .

RUN pip install --no-cache-dir -r python/requirements.txt

# Compile the C++ core + bindings into odelab_py*.so, in place in python/.
RUN cd python && python setup.py build_ext --inplace

WORKDIR /app/python

# Render (and most PaaS providers) inject $PORT at runtime; default to
# 8000 for local `docker run`.
ENV PORT=8000
EXPOSE 8000
CMD ["sh", "-c", "uvicorn server:app --host 0.0.0.0 --port ${PORT}"]
