const udp = require('dgram')
const Bumper = require('./Bumper')

const SERVER_PORT = process.env.SERVER_PORT || 8889
const NUM_BOARDS = 22
const BUMPERS_PER_BOARDS = 8

// udp socket
const client = udp.createSocket('udp4')
client.setMaxListeners(NUM_BOARDS * BUMPERS_PER_BOARDS + 1)
client.bind(SERVER_PORT)
client.on('listening', () => {
  const address = client.address()
  console.log(`server listening ${address.address}:${address.port}`)
})

// bumpers
const bumpers = []
for (let i = 1; i <= NUM_BOARDS; i++) {
  for (let j = 0; j < BUMPERS_PER_BOARDS; j++) {
    const bumper = new Bumper(i, j, client)
    bumpers.push(bumper)
  }
}
bumpers.map(bumper => {
  bumper.on('press', () => {
    console.log(`Bumper ${bumper.getId()} pressed`)
  })
  bumper.on('release', () => {
    console.log(`Bumper ${bumper.getId()} released`)
  })
  bumper.on('error', err => {
    // console.log(err)
  })
})

let i = true
setInterval(() => {
  bumpers.map(bumper => {
    bumper.rgb(i ? 65535 : 0, i ? 65535 : 0, i ? 65535 : 0)
    bumper.buzz(i)
  })
  i = !i
}, 2000)
