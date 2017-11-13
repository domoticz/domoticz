const express = require('express')
const bodyParser = require('body-parser');
const app = express()
const port = 3000

app.use(bodyParser.json());
// app.use(bodyParser.raw({ type: () => true }));

app.get('/testget', (request, response) => {
    console.log('/testget');
    const p = request.query.p || '';
    response.send({
        title: 'GET',
        body: 'This is a GET test',
        p: p
    });
});

app.post('/testpost', (request, response) => {
    console.log('/testpost');
    // console.log(request.body.toString('utf8'));

    console.log(request.query);
    console.log(request.rawHeaders);
    const p = request.query.p || '';

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
