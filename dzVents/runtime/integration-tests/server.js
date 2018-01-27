const express = require('express')
const bodyParser = require('body-parser');
const app = express()
const port = 4000

app.use(bodyParser.json());
// app.use(bodyParser.raw({ type: () => true }));

app.get('/testget', (request, response) => {
    console.log('/testget');
    console.log(request.query)

    const p = request.query.p || '';
    response.send({
        title: 'GET',
        body: 'This is a GET test',
        p: p
    });
});

app.post('/testpost', (request, response) => {
    console.log('/testpost');
    console.log(request.body);
    console.log(request.headers);

    const p = request.body.p || '';

    response.send({
        title: 'POST',
        body: 'This is a POST test',
        p: p
    });
});

app.listen(port, (err) => {
  if (err) {
    return console.log('something bad happened', err)
  }

  console.log(`server is listening on ${port}`)
})
