import Phaser from "phaser";

const COLOR_PRIMARY = 0x4e342e;
const COLOR_LIGHT = 0x7b5e57;
const COLOR_DARK = 0x260e04;

class EditorScene extends Phaser.Scene {
  constructor() {
    super("EditorScene");
  }


 
  loadTemplates = () => {
    this.load.baseURL = "resources/images/";

    // this.renderConfig = getRendererConfig("Sprite2D");

    // this.objectTemplates

  };

  createTileMenu() {
    this.print = this.add.text(0, 0, "");

    var tabs = this.rexUI.add.tabs({
        x: 0,
        y: 300,

        panel: this.rexUI.add.gridTable({
          background: this.rexUI.add.roundRectangle(
            0,
            0,
            20,
            10,
            10,
            COLOR_PRIMARY
          ),

          table: {
            width: 250,
            height: 400,

            cellWidth: 120,
            cellHeight: 60,
            columns: 2,
            mask: {
              padding: 2,
            },
          },

          slider: {
            track: this.rexUI.add.rectangle(0, 0, 20, 10, 10, COLOR_DARK),
            thumb: this.rexUI.add.rectangle(0, 0, 0, 0, 9, COLOR_LIGHT),
          },

          //scroller: true,

          createCellContainerCallback: function (cell) {
            var scene = cell.scene,
              width = cell.width,
              height = cell.height,
              item = cell.item,
              index = cell.index;
            return scene.rexUI.add.label({
              width: width,
              height: height,

              background: scene.rexUI.add
                .roundRectangle(0, 0, 20, 20, 0)
                .setStrokeStyle(2, COLOR_DARK),
              icon: scene.rexUI.add.roundRectangle(
                0,
                0,
                20,
                20,
                10,
                item.color
              ),
              text: scene.add.text(0, 0, item.id),

              space: {
                icon: 10,
                left: 15,
              },
            });
          },
        }),

        // leftButtons: [
        //   this.createButton(this, 2, "AA"),
        //   this.createButton(this, 2, "BB"),
        //   this.createButton(this, 2, "CC"),
        //   this.createButton(this, 2, "DD"),
        // ],

        rightButtons: [this.createButton(this, 0, "+"), this.createButton(this, 0, "-")],

        space: {
          leftButtonsOffset: 20,
          rightButtonsOffset: 30,

          leftButton: 1,
        },
      })
      .layout();
    //.drawBounds(this.add.graphics(), 0xff0000);
    const items = this.createItems();
    tabs.on(
      "button.click",
      function (button, groupName, index) {
        switch (groupName) {
          case "left":
            // Highlight button
            if (this._prevTypeButton) {
              this._prevTypeButton
                .getElement("background")
                .setFillStyle(COLOR_DARK);
            }
            button.getElement("background").setFillStyle(COLOR_PRIMARY);
            this._prevTypeButton = button;
            if (this._prevSortButton === undefined) {
              return;
            }
            break;

          case "right":
            // Highlight button
            if (this._prevSortButton) {
              this._prevSortButton
                .getElement("background")
                .setFillStyle(COLOR_DARK);
            }
            button.getElement("background").setFillStyle(COLOR_PRIMARY);
            this._prevSortButton = button;
            if (this._prevTypeButton === undefined) {
              return;
            }
            break;
        }

        // Load items into grid table
        this.getElement("panel").setItems(items).scrollToTop();
      },
      tabs
    );

    // Grid table
    tabs
      .getElement("panel")
      .on(
        "cell.click",
        function (cellContainer, cellIndex) {
          this.print.text += cellIndex + ": " + cellContainer.text + "\n";
        },
        this
      )
      .on(
        "cell.over",
        function (cellContainer, cellIndex) {
          cellContainer
            .getElement("background")
            .setStrokeStyle(2, COLOR_LIGHT)
            .setDepth(1);
        },
        this
      )
      .on(
        "cell.out",
        function (cellContainer, cellIndex) {
          cellContainer
            .getElement("background")
            .setStrokeStyle(2, COLOR_DARK)
            .setDepth(0);
        },
        this
      );

    tabs.emitButtonClick("left", 0).emitButtonClick("right", 0);
  }

  createItems(count) {
    var TYPE = ["AA", "BB", "CC", "DD"];

    const items = [];
    // Create a collection
    for (var i = 0; i < count; i++) {
      items.push({
        type: TYPE[i % 4],
        id: i,
        color: 0xffffff,
      });
    }
    return items;
  }

  createButton(scene, direction, text) {
    var radius;
    switch (direction) {
      case 0: // Right
        radius = {
          tr: 20,
          br: 20,
        };
        break;
      case 2: // Left
        radius = {
          tl: 20,
          bl: 20,
        };
        break;
    }
    return scene.rexUI.add.label({
      width: 50,
      height: 40,
      background: scene.rexUI.add.roundRectangle(
        0,
        0,
        50,
        50,
        radius,
        COLOR_DARK
      ),
      text: scene.add.text(0, 0, text, {
        fontSize: "18pt",
      }),
      space: {
        left: 10,
      },
    });
  }

  init = (data) => {

    this.editorState = data.editorState;

  }

  preload() {
    console.log("Editor Scene - Preload");

    // load object images
    this.load.scenePlugin({
      key: "rexuiplugin",
      url: "https://raw.githubusercontent.com/rexrainbow/phaser3-rex-notes/master/dist/rexuiplugin.min.js",
      sceneKey: "rexUI",
    });

    this.loadTemplates(this.editorState.objectTemplates);
  }

  create() {
    console.log("Editor Scene - Create");

    this.text = this.add.text(
      this.cameras.main.width / 2,
      this.cameras.main.height / 2,
      "Loading Level Editor",
      {
        font: "32px Arial",
        fill: "#ff0044",
        align: "center",
      }
    );

    //this.createTileMenu();
  }

  update() {
    this.text.setX(this.cameras.main.width / 2);
    this.text.setY(this.cameras.main.height / 2);
    this.text.setOrigin(0.5, 0.5);
  }
}

export default EditorScene;