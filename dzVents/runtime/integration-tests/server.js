const express = require('express')
const bodyParser = require('body-parser');
const app = express()
const port = 3000

app.use(bodyParser.json());
// app.use(bodyParser.raw({ type: () => true }));

app.get('/testget', (request, response) => {
    console.log('/testget');
    response.send({
        title: 'GET',
        body: 'This is a GET test'
    });
});

app.post('/testpost', (request, response) => {
    console.log('/testpost');
    // console.log(request.body.toString('utf8'));

    console.log(request.query);
    console.log(request.rawHeaders);

    response.send({
        title: 'POST',
        body: 'This is a POST test',
        params: request.query.testData
    });
});

app.listen(port, (err) => {
  if (err) {
    return console.log('something bad happened', err)
  }

  console.log(`server is listening on ${port}`)
})
