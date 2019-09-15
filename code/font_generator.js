const section = document.querySelector("section");
const reset   = document.querySelector("button.reset");
const clear = document.querySelector("button.clear");

function newCanvas() {
  clearCanvas();
  let dimensions = getDimensions();
  setCanvas(dimensions[0], dimensions[1]);
}

function clearCanvas() {
  while (section.firstChild) {
    section.removeChild(section.firstChild);
  }
}

function clearCells() {}

function setCanvas(height, width) {
  let size = 50;

  for (let i = 0; i < height; i++) {
    let row = document.createElement("div");
    row.classList.add("row");

    for (let j = 0; j < width; j++) {
      let square = buildSquare(size);

      row.appendChild(square);
    }

    section.appendChild(row);
  }
}

function buildSquare(size) {
  let square = document.createElement("div");

  square.classList.add("square");
  square.setAttribute("style", `width: ${size}px; height: ${size}px`);

  square.addEventListener("click", changeColor);


  return square;
}

function canvasHeight() {
  return parseInt(getComputedStyle(section).getPropertyValue("height"));
}

function getDimensions() {
  let width = prompt("How wide do you want your canvas?", 8);
  let height = prompt("How tall do you want your canvas?", 8);

  return [height, width];
}

function changeColor() {
  this.classList.add("colored");
}

reset.addEventListener("click", newCanvas);

setCanvas();